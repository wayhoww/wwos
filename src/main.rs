#![no_std]
#![no_main]

mod bootloaders;
mod drivers;
mod library;

extern crate alloc;

use core::borrow::BorrowMut;
use core::cell::RefCell;
use core::fmt::Write;
use drivers::Serial32Driver;
use library::{get_serial32_drivers, initialize_hardwares, DeviceTree};
use log::*;

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
                "{} - {}",
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

    let pitch = video.get_pitch();

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

// this function shall be called only once, in one thread
unsafe fn initialize_logging_system() {
    let mut drivers = get_serial32_drivers();
    if drivers.len() > 0 {
        static mut UART_LOGGER: Option<UartLogger> = None;
        UART_LOGGER = Some(UartLogger::new(drivers.remove(0)));
        log::set_logger_racy(UART_LOGGER.as_ref().unwrap()).unwrap();
        log::set_max_level(log::LevelFilter::Info);
    }
}

static mut DEVICE_TREE: Option<DeviceTree> = None;

#[no_mangle]
fn kmain(arg0: usize) -> ! {
    unsafe {
        DEVICE_TREE = Some(DeviceTree::from_memory(arg0 as *const u8));

        // writeln!(Uart, "{:#?}", DEVICE_TREE).unwrap();

        initialize_hardwares(DEVICE_TREE.as_ref().unwrap());
        initialize_logging_system();
    }

    info!("Hello, world!");
    loop {}
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
