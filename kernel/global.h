#ifndef _WWOS_KERNEL_GLOBAL_H
#define _WWOS_KERNEL_GLOBAL_H

#include "aarch64/memory.h"
#include "memory.h"

namespace wwos { class allocator; }

namespace wwos::kernel {
    class physical_memory_page_allocator;
    class pl011_driver;

    extern wwos::allocator* kallocator;

    extern physical_memory_page_allocator* pallocator;
    extern translation_table_kernel* ttkernel;
    extern pl011_driver* g_uart;
}

#endif