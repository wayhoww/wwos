.section ".text.wwos.app.start"

.global _start

_start:
    LDR     x9, =_wwos_application_stack_top
    MOV     sp, x9
    BL      _wwos_runtime_entry

