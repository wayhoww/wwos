#![no_std]
#![no_main]

use core::arch::asm;

#[no_mangle]
pub extern "C" fn _start() -> ! {
    main();
    loop {}
}

fn main() {
    system_call(123, 234);
    system_call(234, 456);
    loop {}
}

pub fn system_call(call_id: u64, arg: u64) {
    unsafe {
        asm!(
            "mov x10, {call_id}",
            "mov x11, {arg}",
            "svc #0",
            call_id = in(reg) call_id,
            arg = in(reg) arg,
        )
    }
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
