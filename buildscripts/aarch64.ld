ENTRY(_start)
SECTIONS {
    . = BOOT_OFFSET;
    __wwos_kernel_binary_begin_mark = .;
    
    .text.wwos.boot : { *(.text.wwos.boot) }            // booting
    
    . = ALIGN(64K);
    __wwos_kernel_shared_begin_mark = .;
    .text.wwos.shared : { *(.text.wwos.shared) }        // codes for switch between el0 & el1
    . = ALIGN(64K);
    . = . + 512K;                                       // stack for exception handler
    __wwos_kernel_shared_end_mark = .;  

    .text : { *(.text), *(.text.*) }
    .data : { *(.data), *(.data.*) }
    .rodata.wwos.blob : { *(.rodata.wwos.blob) }
    .rodata : { *(.rodata), *(.rodata.*) }
    .bss : { *(.bss), *(.bss.*) }
    .rela : { *(.rela), *(.rela.*) }

    .eh_frame_hdr : { *(.eh_frame_hdr) }
    .eh_frame : { *(.eh_frame) }
    .shstrtab : { *(.shstrtab) }
    .strtab : { *(.strtab) }
    .symtab : { *(.symtab) }
    .symtab_shndx : { *(.symtab_shndx) }

    .debug_abbrev : { *(.debug_abbrev) }
    .debug_info : { *(.debug_info) }
    .debug_aranges : { *(.debug_aranges) }
    .debug_ranges : { *(.debug_ranges) }
    .debug_str : { *(.debug_str) }
    .comment : { *(.comment) }
    .debug_frame : { *(.debug_frame) }
    .debug_line : { *(.debug_line) }
    .debug_loc : { *(.debug_loc) }
   
    .other : { *(*) }                                   // wildcard, to avoid sections being placed before begin mark or after end mark

    . = ALIGN(8);
    . = . + 2M;                                         // stack for the kernel
    stack_top = .; 

    . = ALIGN(64K);
    __wwos_kernel_binary_end_mark = .;
}

