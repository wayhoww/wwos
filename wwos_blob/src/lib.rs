#![no_std]

use core::arch::global_asm;

global_asm!(
    r#"
    .balign 8
    .section ".rodata.wwos.blob"
    .global __wwos_blob_start_mark
    __wwos_blob_start_mark:

    .incbin "intermediate/wwos.blob"

    .balign 8
    .section ".rodata.wwos.blob"
    .global __wwos_blob_end_mark
    __wwos_blob_end_mark:
"#
);

pub fn get_wwos_blob() -> &'static [u8] {
    extern "C" {
        static __wwos_blob_start_mark: u8;
        static __wwos_blob_end_mark: u8;
    }

    unsafe {
        let start = &__wwos_blob_start_mark as *const u8 as usize;
        let end = &__wwos_blob_end_mark as *const u8 as usize;
        core::slice::from_raw_parts(start as *const u8, end - start)
    }
}
