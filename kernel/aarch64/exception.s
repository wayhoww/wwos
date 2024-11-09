.section ".text.wwos.exception"

protect_and_handle_exception:
    STP     x2, x3,   [sp, #0x10]
    STP     x4, x5,   [sp, #0x20]
    STP     x6, x7,   [sp, #0x30]
    STP     x8, x9,   [sp, #0x40]
    STP     x10, x11, [sp, #0x50]
    STP     x12, x13, [sp, #0x60]
    STP     x14, x15, [sp, #0x70]
    STP     x16, x17, [sp, #0x80]
    STP     x18, x19, [sp, #0x90]
    STP     x20, x21, [sp, #0xA0]
    STP     x22, x23, [sp, #0xB0]
    STP     x24, x25, [sp, #0xC0]
    STP     x26, x27, [sp, #0xD0]
    STP     x28, x29, [sp, #0xE0]
    STR     x30,      [sp, #0xF0]

    MOV     x3, x0;   // i.e. exception source

    MRS     x0, SPSR_EL1;
    MRS     x1, ELR_EL1;
    STP     x0, x1, [sp, #0xF8];

    MRS     x0, SP_EL0;
    STR     x0, [sp, #0x108]
    
    MOV     x0, x10;
    MOV     x1, x11;
    MOV     x2, sp;
    B       wwos_aarch64_handle_exception;


.global aarch64_exception_vector_table;

.balign 0x80
aarch64_exception_vector_table:

.balign 0x80
current_el_sp0_sync:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 0;
    B       protect_and_handle_exception;
.balign 0x80
current_el_sp0_irq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 1;
    B       protect_and_handle_exception;
.balign 0x80
current_el_sp0_fiq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 2;
    B       protect_and_handle_exception;
.balign 0x80
current_el_sp0_serror:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 3;
    B       protect_and_handle_exception;

.balign 0x80
current_el_sp1_sync:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 4;
    B       protect_and_handle_exception;
.balign 0x80
current_el_sp1_irq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 5;
    B       protect_and_handle_exception;
.balign 0x80
current_el_sp1_fiq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 6;
    B       protect_and_handle_exception;
.balign 0x80
current_el_sp1_serror:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 7;
    B       protect_and_handle_exception;

.balign 0x80
lower_el_aarch64_sync:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 8;
    B       protect_and_handle_exception;
.balign 0x80
lower_el_aarch64_irq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 9;
    B       protect_and_handle_exception;
.balign 0x80
lower_el_aarch64_fiq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 10;
    B       protect_and_handle_exception;
.balign 0x80
lower_el_aarch64_serror:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 11;
    B       protect_and_handle_exception;

.balign 0x80
lower_el_aarch32_sync:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 12;
    B       protect_and_handle_exception;
.balign 0x80
lower_el_aarch32_irq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 13;
    B       protect_and_handle_exception;
.balign 0x80
lower_el_aarch32_fiq:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 14;
    B       protect_and_handle_exception;
.balign 0x80
lower_el_aarch32_serror:
    SUB     sp, sp, #0x110
    STP     x0, x1,   [sp, #0x00]
    MOV     x0, 15;
    B       protect_and_handle_exception;
