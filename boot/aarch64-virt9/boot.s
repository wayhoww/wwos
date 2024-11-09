.section ".text.wwos.boot"

.global _start

_start:
    LDR     x1, =stack_top
    MOV     sp, x1
    BL      loader_main


.section ".data.wwos.kernel"
.INCBIN "kernel/kernel.img"

.section ".data.wwos.memdisk"
.INCBIN "memdisk.wwfs"
