#![no_std]
#![no_main]
#![allow(dead_code)]

mod arch;
mod boot;
mod kernel_drivers;
mod library;
mod memory;

extern crate alloc;

use alloc::{boxed::Box, rc::Rc};
use alloc::vec::Vec;
use arch::{handle_exception, initialize_arch, start_userspace_execution, system_call, DataAbortHandler, DATA_ABORT_HANDLER, USER_DATA_ABORT_HANDLER};
use core::arch::asm;
use core::cell::RefCell;
use core::fmt::Write;
use core::ptr::{addr_of, read, read_volatile};
use kernel_drivers::Serial32Driver;
use library::{initialize_kernel_hardwares, DeviceTree, DriverInstance};
use log::info;
use memory::{
    MemoryBlock, MemoryBlockAttributes, TranslationTable, TranslationTableAarch64, KERNEL_MEMORY_ALLOCATOR, PAGE_SIZE
};

use log::{Level, Metadata, Record};

struct UartLogger {
    uart: RefCell<&'static mut dyn Serial32Driver>,
}

impl UartLogger {
    fn new(uart: &'static mut dyn Serial32Driver) -> Self {
        Self {
            uart: RefCell::new(uart),
        }
    }
}

unsafe impl Sync for UartLogger {}
unsafe impl Send for UartLogger {}

impl log::Log for UartLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= Level::Info
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            writeln!(
                self.uart.borrow_mut(),
                "{} - {}\r",
                record.level(),
                record.args()
            )
            .unwrap();
        }
    }

    fn flush(&self) {}
}

fn draw_pixel(video: &mut kernel_drivers::VideoCoreDriver, x: i32, y: i32, color: u32) {
    if x < 0 || x >= video.get_width() || y < 0 || y >= video.get_height() {
        return;
    }

    let fb = video.get_mut_framebuffer();
    let pitch = video.get_pitch();
    unsafe {
        core::ptr::write_volatile(fb.offset((y * pitch + x * 4) as isize) as *mut u32, color);
    }
}

fn draw_rect(
    video: &mut kernel_drivers::VideoCoreDriver,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    color: u32,
) {
    for i in 0..height {
        for j in 0..width {
            draw_pixel(video, x + j, y + i, color);
        }
    }
}

// this function shall be called only once, in one thread
fn initialize_logging_system() {
    let driver_instances = unsafe { KERNEL_DRIVER_INSTANCES.as_mut().unwrap() };
    let mut serial32_drivers: Vec<_> = driver_instances
        .iter_mut()
        .flat_map(|inst| inst.driver.as_serial32_mut())
        .collect();

    if !serial32_drivers.is_empty() {
        static mut UART_LOGGER: Option<UartLogger> = None;
        unsafe {
            UART_LOGGER = Some(UartLogger::new(serial32_drivers.remove(0)));
        }
        unsafe {
            log::set_logger_racy(UART_LOGGER.as_ref().unwrap()).unwrap();
        }
        log::set_max_level(log::LevelFilter::Info);
    }
}

fn usermain() {
    info!("Hello, world from userspace!");

    system_call(0, 12);

    info!("Hello, world from userspace!");

    loop {}
}

fn set_tcr_el1(tcr: u64) {
    unsafe {
        asm!("msr tcr_el1, {x}", x = in(reg) tcr);
    }
}

fn get_tcr_el1() -> u64 {
    let mut tcr_el1: u64;
    unsafe {
        asm!("mrs {x}, tcr_el1", x = out(reg) tcr_el1);
    }
    tcr_el1
}

fn get_sctlr_el1() -> u64 {
    let mut sctlr_el1: u64;
    unsafe {
        asm!("mrs {x}, sctlr_el1", x = out(reg) sctlr_el1);
    }
    sctlr_el1
}

fn set_sctlr_el1(sctlr: u64) {
    unsafe {
        asm!("msr sctlr_el1, {x}", x = in(reg) sctlr);
    }
}

fn set_ttbr0_el1(ttbr: u64) {
    unsafe {
        asm!("msr ttbr0_el1, {x}", x = in(reg) ttbr);
    }
}

fn get_id_aa64mmfr0_el1() -> u64 {
    let mut id_aa64mmfr0_el1: u64;
    unsafe {
        asm!("mrs {x}, id_aa64mmfr0_el1", x = out(reg) id_aa64mmfr0_el1);
    }
    id_aa64mmfr0_el1
}

fn set_bits(val: u64, bits: u64, begin: u64, end: u64) -> u64 {
    let mask = (1 << (end - begin)) - 1;
    (val & !(mask << begin)) | (bits << begin)
}

static mut KERNEL_DRIVER_INSTANCES: Option<Vec<DriverInstance>> = None;

#[derive(Debug)]
struct PhysicalMemoryPageNode {
    addr: usize,
    size: usize,
}

#[derive(Debug)]
struct PhysicalMemoryPageAllocator {
    freelist: Vec<PhysicalMemoryPageNode>,
}

impl PhysicalMemoryPageAllocator {
    fn new(begin: usize, size: usize, page_size: usize) -> Self {
        // make sure all are aligned
        if begin % page_size != 0 || size % page_size != 0 {
            panic!("misaligned memory");
        }

        let mut freelist = Vec::new();
        freelist.push(PhysicalMemoryPageNode {
            addr: begin,
            size: size,
        });

        Self { freelist: freelist }
    }

    fn alloc(&mut self) -> Option<usize> {
        if let Some(node) = self.freelist.last_mut() {
            let addr = node.addr;
            node.size -= PAGE_SIZE;
            node.addr += PAGE_SIZE;

            if node.size == 0 {
                self.freelist.pop();
            }

            Some(addr)
        } else {
            None
        }
    }

    fn alloc_specific_page(&mut self, addr: usize) -> bool {
        let mut i = 0;
        for node in self.freelist.iter() {
            if node.addr > addr {
                break;
            }
            i += 1;
        }

        if i < self.freelist.len() && self.freelist[i].addr + self.freelist[i].size >= addr + PAGE_SIZE {
            if self.freelist[i].size == PAGE_SIZE {
                self.freelist.remove(i);
                true
            } else {
                if self.freelist[i].addr == addr {
                    self.freelist[i].addr += PAGE_SIZE;
                    self.freelist[i].size -= PAGE_SIZE;
                    true
                } else if self.freelist[i].addr + self.freelist[i].size == addr + PAGE_SIZE {
                    self.freelist[i].size -= PAGE_SIZE;
                    true
                } else {
                    // split into two
                    let chunk_size = self.freelist[i].size;
                    self.freelist[i].size = addr - self.freelist[i].addr;
                    self.freelist.insert(
                        i + 1,
                        PhysicalMemoryPageNode {
                            addr: addr + PAGE_SIZE,
                            size: chunk_size - self.freelist[i].size - PAGE_SIZE,
                        },
                    );
                    true
                }
            }
        } else {
            false
        }
    }

    fn free(&mut self, addr: usize) {
        let mut i = 0;
        for node in self.freelist.iter() {
            if node.addr > addr {
                break;
            }
            i += 1;
        }

        let left_contiguous =
            i as isize - 1 >= 0 && self.freelist[i - 1].addr + self.freelist[i - 1].size == addr;
        let right_contiguous = i < self.freelist.len() && addr + PAGE_SIZE == self.freelist[i].addr;

        if left_contiguous && right_contiguous {
            self.freelist[i - 1].size += self.freelist[i].size + PAGE_SIZE;
            self.freelist.remove(i);
        } else if left_contiguous {
            self.freelist[i - 1].size += PAGE_SIZE;
        } else if right_contiguous {
            self.freelist[i].addr -= PAGE_SIZE;
            self.freelist[i].size += PAGE_SIZE;
        } else {
            self.freelist.insert(
                i,
                PhysicalMemoryPageNode {
                    addr: addr,
                    size: PAGE_SIZE,
                },
            );
        }
    }
}

struct KernelPageAllocator {
    table: Box<dyn TranslationTable>,
    physical_allocator: Rc<RefCell<PhysicalMemoryPageAllocator>>,
}

impl DataAbortHandler for KernelPageAllocator {
    fn handle_data_abort(&mut self, address: usize) -> bool {
        let aligned_address = address & !(PAGE_SIZE - 1);
        
        let block = MemoryBlock {
            virtual_address: aligned_address,
            physical_address: aligned_address,
            attr: MemoryBlockAttributes {
                permission: memory::MemoryPermission::KernelRWX,
                mtype: memory::MemoryType::Normal,
            },
        };

        self.table.add_block(&block);
        self.table.activate();

        true
    }
}

const PROCESS_BINARY_BEGIN: usize = 0x400000;
const SHARED_REGION_BEGIN: usize = PAGE_SIZE;

extern "C" fn test_userspace_execution() {
    loop {
        unsafe { asm!("b .") };
    }
}


struct UserProcessDataAbortHandler {
    physical_allocator: Rc<RefCell<PhysicalMemoryPageAllocator>>,
    translation_table: Box<dyn TranslationTable>,
}

impl DataAbortHandler for UserProcessDataAbortHandler {
    fn handle_data_abort(&mut self, address: usize) -> bool {
        // check if the address should be accessible to userspace
        
        info!("UserProcessDataAbortHandler::handle_data_abort: address: {:x?}", address);

        let aligned_address = address & !(PAGE_SIZE - 1);
        let physical_address = self.physical_allocator.borrow_mut().alloc();
        

        info!("UserProcessDataAbortHandler::handle_data_abort: aligned_address: {:x?} physical_address: {:x?}", aligned_address, physical_address);

        if physical_address.is_none() {
            return false;
        }


        let block = MemoryBlock {
            virtual_address: aligned_address,
            physical_address: physical_address.unwrap(),
            attr: MemoryBlockAttributes {
                permission: memory::MemoryPermission::KernelRwUserRWX,
                mtype: memory::MemoryType::Normal,
            },
        };

        info!("UserProcessDataAbortHandler::handle_data_abort: block: {:#x?}", block);

        self.translation_table.add_block(&block);

        info!("UserProcessDataAbortHandler::handle_data_abort: added block to translation_table");

        // self.translation_table.activate();

        true
    }
}


static mut KERNEL_MEMORY_BLOCKS: Vec<MemoryBlock> = Vec::new();

#[no_mangle]
pub extern "C" fn kmain(dtb_address: *const u8) -> ! {
    initialize_arch();

    unsafe { KERNEL_MEMORY_ALLOCATOR.initialize() };

    let device_tree = unsafe { DeviceTree::from_memory(dtb_address) };
    unsafe {
        KERNEL_DRIVER_INSTANCES = Some(initialize_kernel_hardwares(&device_tree));
    }

    initialize_logging_system();

    // print all drivers
    let driver_instances = unsafe { KERNEL_DRIVER_INSTANCES.as_ref().unwrap() };
    for driver_instance in driver_instances.iter() {
        info!("{:?}", driver_instance.path);
    }

    let memories: Vec<_> = unsafe {
        KERNEL_DRIVER_INSTANCES
            .as_ref()
            .unwrap()
            .iter()
            .flat_map(|x| x.driver.as_memory())
            .collect()
    };

    let memory = memories[0];

    let memory_end = memory.get_address() + memory.get_size();
    let aligned_memory_end = (memory_end + 8) & !7;
    unsafe {
        KERNEL_MEMORY_ALLOCATOR.extend(aligned_memory_end);
    }

    let upperbound = unsafe { KERNEL_MEMORY_ALLOCATOR.get_used_address_upperbound() };
    
    extern "C" {
        static __wwos_kernel_binary_begin_mark: u64;
    }

    let kernel_eary_memory_begin =
        unsafe { addr_of!(__wwos_kernel_binary_begin_mark) as *const u8 as usize };
    let kernel_kernel_memory_end = upperbound + 8 * 1024 * 1024; // add 4MB, for safety reason

    let aligned_kernel_eary_memory_begin = kernel_eary_memory_begin & !(PAGE_SIZE - 1);
    let aligned_kernel_kernel_memory_end = (kernel_kernel_memory_end + PAGE_SIZE - 1) & !(PAGE_SIZE - 1);

    let page_count = (aligned_kernel_kernel_memory_end - aligned_kernel_eary_memory_begin) / PAGE_SIZE;
    for i in 0..page_count {
        let address = aligned_kernel_eary_memory_begin + i * PAGE_SIZE;
        let block = MemoryBlock {
            virtual_address: address,
            physical_address: address,
            attr: MemoryBlockAttributes {
                permission: memory::MemoryPermission::KernelRWX,
                mtype: memory::MemoryType::Normal,
            },
        };
        unsafe { KERNEL_MEMORY_BLOCKS.push(block) };
    }

    // TODO: assert pages for shared region is not occupied

    extern "C" {
        static __wwos_kernel_shared_begin_mark: u64;
        static __wwos_kernel_shared_end_mark: u64;
    }

    // TODO: assert alignment
    let shared_begin = unsafe { addr_of!(__wwos_kernel_shared_begin_mark) as *const u8 as usize };
    let shared_end = unsafe { addr_of!(__wwos_kernel_shared_end_mark) as *const u8 as usize };
    let shared_page_count = (shared_end - shared_begin + PAGE_SIZE - 1) / PAGE_SIZE;


    // add all of them to the translation table
    for i in 0..shared_page_count {
        let address = shared_begin + i * PAGE_SIZE;
        let block = MemoryBlock {
            virtual_address: SHARED_REGION_BEGIN + i * PAGE_SIZE,
            physical_address: address,
            attr: MemoryBlockAttributes {
                permission: memory::MemoryPermission::KernelRWX,
                mtype: memory::MemoryType::Normal,
            },
        };
        unsafe { KERNEL_MEMORY_BLOCKS.push(block) };
    }
    

    let kernel_memory_mapping = 
        memory::TranslationTableAarch64::from_memory_pages(unsafe { KERNEL_MEMORY_BLOCKS.as_slice() });
        kernel_memory_mapping.activate();

    extern "C" {
        static mut __wwos_kernel_memory_mapping_address: u64;
        static mut __wwos_user_memory_mapping_address: u64;
        static mut __wwos_exception_handler_physical_address: u64;
    }

    // 1. print addr of __wwos_kernel_memory_mapping_address
    // 2. print addr of __wwos_kernel_memory_mapping_address - __wwos_shared_begin_mark
    // 3. print addr of __wwos_kernel_memory_mapping_address - __wwos_shared_begin_mark + SHARED_REGION_BEGIN

    
    let addr_of_wwo_kernel_memory_mapping_address = unsafe { addr_of!(__wwos_kernel_memory_mapping_address) as *const u8 as usize };
    let addr_of_shared_begin_mark = unsafe { addr_of!(__wwos_kernel_shared_begin_mark) as *const u8 as usize };

    // info!("addr_of_wwo_kernel_memory_mapping_address: {:x}", addr_of_wwo_kernel_memory_mapping_address);
    // info!("addr_of_wwo_kernel_memory_mapping_address - addr_of_shared_begin_mark: {:x}", addr_of_wwo_kernel_memory_mapping_address - addr_of_shared_begin_mark);
    // info!("addr_of_wwo_kernel_memory_mapping_address - addr_of_shared_begin_mark + SHARED_REGION_BEGIN: {:x}", addr_of_wwo_kernel_memory_mapping_address - addr_of_shared_begin_mark + SHARED_REGION_BEGIN);

    unsafe {
        __wwos_kernel_memory_mapping_address = kernel_memory_mapping.get_address() as u64;
        __wwos_exception_handler_physical_address = handle_exception as u64;
    }

    let new_upperbound = unsafe { KERNEL_MEMORY_ALLOCATOR.get_used_address_upperbound() };

    if new_upperbound > aligned_kernel_kernel_memory_end {
        panic!("Internal error");
    }

    let physical_allocator = Rc::new(RefCell::new(PhysicalMemoryPageAllocator::new(
        memory.get_address(),
        memory.get_size(),
        PAGE_SIZE,
    )));

    for block in unsafe { KERNEL_MEMORY_BLOCKS.iter() } {
        physical_allocator.borrow_mut().alloc_specific_page(block.physical_address);
    }

    let page_allocator = Box::new(KernelPageAllocator {
        table: Box::new(kernel_memory_mapping),
        physical_allocator: physical_allocator.clone(),
    }) as Box<dyn DataAbortHandler>;

    unsafe { DATA_ABORT_HANDLER = Some(page_allocator) };
    
    
    info!("HELLO");

    system_call(1, 1);


    let blob = wwos_blob::get_wwos_blob();
    info!("blob: {:?}", blob);


    let mut user_blocks: Vec<MemoryBlock> = Vec::new();

    // copy blob to whatever page-aligned address
    let blob_size = blob.len();
    let blob_page_count = (blob_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for i in 0..blob_page_count {
        let page_address = physical_allocator.borrow_mut().alloc().unwrap();
        let virtual_begin = PROCESS_BINARY_BEGIN + i * PAGE_SIZE;
        let blob_begin = i * PAGE_SIZE;
        let copy_size = core::cmp::min(PAGE_SIZE, blob_size - blob_begin);
        // print source & destination address
        info!("blob_begin: {:p}, page_address: {:x}, copy_size: {:x}", unsafe { blob.as_ptr().add(blob_begin) }, page_address, copy_size);
        unsafe {
            core::ptr::copy(
                blob.as_ptr().add(blob_begin),
                page_address as *mut u8,
                copy_size,
            );
        }
        user_blocks.push(MemoryBlock {
            virtual_address: virtual_begin,
            physical_address: page_address,
            attr: MemoryBlockAttributes {
                permission: memory::MemoryPermission::KernelRwUserRWX,
                mtype: memory::MemoryType::Normal,
            },
        });
    }

    // add shared region translation table
    for i in 0..shared_page_count {
        let address = shared_begin + i * PAGE_SIZE;
        let block = MemoryBlock {
            virtual_address: SHARED_REGION_BEGIN + i * PAGE_SIZE,
            physical_address: address,
            attr: MemoryBlockAttributes {
                permission: memory::MemoryPermission::KernelRWX,
                mtype: memory::MemoryType::Normal,
            },
        };
        user_blocks.push(block);
    }

    let user_memory_mapping = memory::TranslationTableAarch64::from_memory_pages(&user_blocks);

    let memory_mapping_address = user_memory_mapping.get_address();

    unsafe {
        __wwos_user_memory_mapping_address = memory_mapping_address as u64;
    }
    
    let user_data_abort_handler = Box::new(UserProcessDataAbortHandler {
        physical_allocator: physical_allocator.clone(),
        translation_table: Box::new(user_memory_mapping),
    }) as Box<dyn DataAbortHandler>;

    unsafe { USER_DATA_ABORT_HANDLER = Some(user_data_abort_handler) };

    start_userspace_execution(PROCESS_BINARY_BEGIN, PROCESS_BINARY_BEGIN + blob_size + 2 * 4096 * 4096, memory_mapping_address);
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    info!("Panic: {:?}", _info);
    loop {}
}
