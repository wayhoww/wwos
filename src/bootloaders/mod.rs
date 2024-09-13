#[cfg(WWOS_BOARD = "qemu_aarch64_virt9")]
core::arch::global_asm!(
    r#"
.section ".text.boot"  

.global _start

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

1:  wfe
    b       1b

2:  ldr     x1, =stack_top
    mov     sp, x1

4:  mov     x0, #0x40000000
    bl      kmain
    b       1b
"#
);

#[cfg(WWOS_BOARD = "raspi4b")]
core::arch::global_asm!(
    r#"
.section ".text.boot"  

.global _start

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

1:  wfe
    b       1b

2:  ldr     x1, =stack_top
    mov     sp, x1

4:  bl      kmain
    b       1b
"#
);
