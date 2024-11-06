.section ".text.wwos.start"  

.global _start

_start:
    LDR     x9, =stack_top
    MOV     sp, x9
    BL      kmain

