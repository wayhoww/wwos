#![no_std]
#![no_main]
#![allow(dead_code)]

mod arch;
mod boot;
mod drivers;
mod library;

extern crate alloc;

use arch::{get_current_el, initialize_arch, start_userspace_execution, system_call};
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


fn initialize_translation_table() {
    
}

#[no_mangle]
fn kmain(arg0: usize) -> ! {



    // initialize_arch();

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
