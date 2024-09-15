#[cfg(WWOS_BOARD = "qemu_aarch64_virt9")]
core::arch::global_asm!(
    r#"
.section ".text.boot"  

.global _start

_start:
    ldr     x1, =stack_top
    mov     sp, x1
    mov     x0, #0x40000000
    bl      kmain
"#
);

#[cfg(WWOS_BOARD = "raspi4b")]
core::arch::global_asm!(
    r#"
.section ".text.boot"  

.global _start

_start:
    ldr     x1, =stack_top
    mov     sp, x1
    bl      kmain
"#
);
