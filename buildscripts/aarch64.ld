ENTRY(_start)
SECTIONS {
    . = BOOT_OFFSET;
    .text.boot : { *(.text.boot) }
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata.*) }
    .bss : { *(.bss) }

    . = ALIGN(8);
    . = . + 2M;
    stack_top = .;
}

