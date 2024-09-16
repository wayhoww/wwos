#![no_std]
#![no_main]
#![allow(dead_code)]

mod arch;
mod boot;
mod drivers;
mod library;
mod memory;

extern crate alloc;

use alloc::alloc::{alloc, dealloc};
use alloc::vec::{self, Vec};
use arch::{initialize_arch, system_call};
use memory::{set_translation_table, MemoryPage, MemoryType, KERNEL_MEMORY_ALLOCATOR};
use core::alloc::Layout;
use core::arch::asm;
use core::cell::RefCell;
use core::fmt::Write;
use core::ptr::{read_volatile, write_volatile};
use drivers::Serial32Driver;
use library::{get_serial32_drivers, initialize_hardwares, DeviceTree, DeviceTreeMatch};
use log::info;

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

fn draw_pixel(video: &mut drivers::VideoCoreDriver, x: i32, y: i32, color: u32) {
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
    video: &mut drivers::VideoCoreDriver,
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

struct Uart;

impl core::fmt::Write for Uart {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            unsafe { write_volatile(0xfe201000usize as *mut u8, c as u8) }
        }

        Ok(())
    }
}

// this function shall be called only once, in one thread
unsafe fn initialize_logging_system() {
    let mut drivers = get_serial32_drivers();
    if !drivers.is_empty() {
        static mut UART_LOGGER: Option<UartLogger> = None;
        UART_LOGGER = Some(UartLogger::new(drivers.remove(0)));
        log::set_logger_racy(UART_LOGGER.as_ref().unwrap()).unwrap();
        log::set_max_level(log::LevelFilter::Info);
    }
}

static mut DEVICE_TREE: Option<DeviceTree> = None;

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



pub static mut MEMORY_PAGES: Vec<MemoryPage> = Vec::new();

unsafe fn set_translation_table2() {
    MEMORY_PAGES.push(
        MemoryPage {
            virtual_address: 0xfe201000 & !((1 << 12) - 1),
            physical_address: 0xfe201000 & !((1 << 12) - 1),
            user_readable: true,
            user_writable: true,
            kernel_readable: true,
            kernel_writable: true,
            memory_type: MemoryType::Normal,
        }
    );

    for i in 0..512 * 8 {
        MEMORY_PAGES.push(MemoryPage {
            virtual_address: i * (1 << 12),
            physical_address: i * (1 << 12),
            user_readable: true,
            user_writable: true,
            kernel_readable: true,
            kernel_writable: true,
            memory_type: MemoryType::Normal,
        });
    }


    for i in 0..128 {
        MEMORY_PAGES.push(MemoryPage {
            virtual_address: 0x2000_0000 + i * (1 << 12),
            physical_address: 0x2000_0000 + i * (1 << 12),
            user_readable: true,
            user_writable: true,
            kernel_readable: true,
            kernel_writable: true,
            memory_type: MemoryType::Normal,
        });
    }
}

#[no_mangle]
fn kmain(arg0: usize) -> ! {
    unsafe { KERNEL_MEMORY_ALLOCATOR.initialize() };

    write!(Uart, "Hello, world from kernel!\r\n").unwrap();

    unsafe { 
        let layout = Layout::from_size_align(1024, 4).unwrap();
        let ptr = alloc(layout);
        dealloc(ptr, layout);


        let layout = Layout::from_size_align(1024, 4).unwrap();
        let ptr = alloc(layout);
        dealloc(ptr, layout);


        let layout = Layout::from_size_align(1024, 4).unwrap();
        let ptr = alloc(layout);
        dealloc(ptr, layout);
    }

    initialize_arch();

    // unsafe {
    //     set_translation_table2();
    //     set_translation_table(&MEMORY_PAGES);
    // }

    write!(Uart, "Hello, world from kernel!\r\n").unwrap();

    // unsafe {
    //     let addr = 0x30000000usize as *mut u32;
    //     *addr = 0x12345678;
    // }
    
    unsafe {
        DEVICE_TREE = Some(DeviceTree::from_memory(arg0 as *const u8));

        initialize_hardwares(DEVICE_TREE.as_ref().unwrap());
        initialize_logging_system();

        let root = &DEVICE_TREE.as_ref().unwrap().root;
        let mem = DEVICE_TREE.as_ref().unwrap().find_nodes_by_property("device_type", "memory".as_bytes(), DeviceTreeMatch::Contains)[0];
        let (address, size) = mem.get_address_size(root).unwrap();
        info!("memory: {:#x} - {:#x}", address, size);

        info!("device: {:#?}", DEVICE_TREE);
    }


    // info!("Hello, world from kernel!");


    // let uart = &mut get_serial32_drivers()[0];
    // write!(uart, "Hello, world from kernel!\r\n").unwrap();


    loop {}
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    write!(Uart, "Panic: {:?}", _info);
    loop {}
}
