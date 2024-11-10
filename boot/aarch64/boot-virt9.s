.section ".text.wwos.boot"

.global _start

_start:
    MRS     x0, MPIDR_EL1
    AND     x0, x0, #3
    CBZ     x0, BEGIN

1:  
    WFE
    B       1b

BEGIN:
    LDR     x1, =stack_top
    MOV     sp, x1 
    BL      loader_main
    

.section ".data.wwos.kernel"
.INCBIN "kernel/kernel.img"

.section ".data.wwos.memdisk"
.INCBIN "memdisk.wwfs"
