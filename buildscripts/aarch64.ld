ENTRY(_start)
SECTIONS {
    . = BOOT_OFFSET;
    .text.boot : { *(.text.boot) }
    .eh_frame_hdr : { *(.eh_frame_hdr) }
    .eh_frame : { *(.eh_frame) }
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata.*) }
    .bss : { *(.bss) }
    
    . = ALIGN(8);
    . = . + 2M;
    stack_top = .;

    . = . + 2M;
    el0_stack_top = .;
}

