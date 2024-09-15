#![no_std]
#![no_main]
#![allow(dead_code)]

mod arch;
mod boot;
mod drivers;
mod library;

extern crate alloc;

use arch::{initialize_arch, system_call};
use core::cell::RefCell;
use core::fmt::Write;
use core::ptr::write_volatile;
use drivers::Serial32Driver;
use library::{get_serial32_drivers, initialize_hardwares, DeviceTree};
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

// #define PF_TYPE_BLOCK      ((uint64_t)1 << 0)
// #define PF_MEM_TYPE_NORMAL ((uint64_t)MT_NORMAL << 2)
// #define PF_READ_WRITE      ((uint64_t)1 << 6)
// #define PF_INNER_SHAREABLE ((uint64_t)3 << 8)
// #define PF_ACCESS_FLAG     ((uint64_t)1 << 10)

// void ttbr0_el2_store(uint64_t ttbr);

// void paging_idmap_setup(void)
// {
//     static uint64_t idmap[64] __attribute__ ((__aligned__(512)));

//     const uint64_t block_size = 0x40000000000ull;
//     const uint64_t block_attr =
//         PF_TYPE_BLOCK |
//         PF_MEM_TYPE_NORMAL |
//         PF_READ_WRITE |
//         PF_INNER_SHAREABLE |
//         PF_ACCESS_FLAG;
//     uint64_t phys = 0;

//     for (size_t i = 0; i < sizeof(idmap)/sizeof(idmap[0]); ++i) {
//         idmap[i] = phys | block_attr;
//         phys += block_size;
//     }

//     ttbr0_el2_store((uint64_t)idmap);
// }

// ttbr0_el2_store:
//     msr ttbr0_el2, x0
//     ret

// fn ttbr0_el1_store(ttbr: u64) {
//     unsafe {
//         asm!("msr ttbr0_el1, {x}", x = in(reg) ttbr);
//     }
// }

// fn initialize_translation_table() {
//     let addr = unsafe { alloc(Layout::from_size_align(64 * 8, 512).unwrap()) as *mut u64 };

//     let block_size = 0x400_0000_0000usize;
//     let block_attr = 0b1 << 0 | 0b10 << 2 | 0b1 << 6 | 0b11 << 8 | 0b1 << 10;
//     let mut phys = 0;

//     for i in 0..64 {
//         unsafe {
//             core::ptr::write_volatile(addr.offset(i as isize), phys as u64 | block_attr);
//         }
//         phys += block_size;
//     }

//     ttbr0_el1_store(addr as u64);
// }

#[no_mangle]
fn kmain(arg0: usize) -> ! {
    initialize_arch();

    unsafe {
        DEVICE_TREE = Some(DeviceTree::from_memory(arg0 as *const u8));

        initialize_hardwares(DEVICE_TREE.as_ref().unwrap());
        initialize_logging_system();
    }

    info!("Hello, world!");

    // initialize_translation_table();

    // unsafe {
    //     DEVICE_TREE = Some(DeviceTree::from_memory(arg0 as *const u8));

    //     initialize_hardwares(DEVICE_TREE.as_ref().unwrap());
    //     initialize_logging_system();

    //     info!("Hello, world!");
    //     let el = get_current_el();
    //     info!("Current EL: {}", el);

    //     // system_call(0, 10);
    //     // system_call(0, 10);
    //     // system_call(0, 10);
    //     // system_call(0, 10);
    // }

    // unsafe {
    //     start_userspace_execution(usermain as usize);
    // }

    loop {}
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    info!("Panic: {:?}", _info);
    loop {}
}
