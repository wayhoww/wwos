#include "aarch64/interrupt.h"
#include "drivers/pl011.h"
#include "wwos/algorithm.h"
#include "wwos/alloc.h"
#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"

#include "process.h"
#include "filesystem.h"
#include "memory.h"
#include "global.h"
#include "arch.h"
#include "wwos/syscall.h"



extern wwos::uint64_t wwos_kernel_begin_mark;
extern wwos::uint64_t wwos_kernel_end_mark;

extern wwos::uint64_t memdisk_blob_begin_mark;

namespace wwos::kernel {

translation_table_kernel* initialize_memory() {
    static translation_table_kernel tt;

    // size: 1GB, begin: 1GB
    constexpr size_t MEMORY_BEGIN = 1 << 30;
    constexpr size_t MEMORY_SIZE = 1 << 30;
    constexpr size_t KERNEL_RESERVED_HEAP = 16 << 20;

    size_t aligned_begin = align_down(reinterpret_cast<uint64_t>(&wwos_kernel_begin_mark), translation_table_kernel::PAGE_SIZE);
    size_t aligned_end = align_up(reinterpret_cast<uint64_t>(&wwos_kernel_end_mark), translation_table_kernel::PAGE_SIZE);

    static allocator allocator(aligned_end, KERNEL_RESERVED_HEAP);
    kallocator = &allocator;

    static physical_memory_page_allocator s_pallocator(MEMORY_BEGIN, MEMORY_SIZE, translation_table_kernel::PAGE_SIZE);
    for(uint64_t va = aligned_begin; va < aligned_end + KERNEL_RESERVED_HEAP; va += translation_table_kernel::PAGE_SIZE) {
        auto pa = va - KA_BEGIN;
        s_pallocator.alloc_specific_page(pa);
        tt.set_page(va, pa);
    }
    pallocator = &s_pallocator;

    return &tt;
}

void initialize_logging() {
    create_shared_file_node("/kernel", fd_type::DIRECTORY);
    create_shared_file_node("/kernel/log", fd_type::FIFO);

    auto sfn = open_shared_file_node(0, "/kernel/log", fd_mode::WRITEONLY);
    wwassert(sfn, "failed to open file");
    kernel_logging_sfn = sfn;
}


void main(wwos::uint64_t pa_memdisk_begin, wwos::uint64_t pa_memdisk_end) {
    ttkernel = initialize_memory();
    
    auto aligned_uart_begin = align_down(0x09000000ull, translation_table_kernel::PAGE_SIZE);
    ttkernel->set_page(aligned_uart_begin, aligned_uart_begin);
    
    auto aligned_gicd_begin = align_down(0x08000000ull, translation_table_kernel::PAGE_SIZE);
    ttkernel->set_page(aligned_gicd_begin, aligned_gicd_begin);

    auto aligned_gicc_begin = align_down(0x08010000ull, translation_table_kernel::PAGE_SIZE);
    ttkernel->set_page(aligned_gicc_begin, aligned_gicc_begin);
    
    auto aligned_memdisk_begin = align_down(pa_memdisk_begin, translation_table_kernel::PAGE_SIZE);
    auto aligned_memdisk_end = align_up(pa_memdisk_end, translation_table_kernel::PAGE_SIZE);
    for(uint64_t pa = aligned_memdisk_begin; pa < aligned_memdisk_end; pa += translation_table_kernel::PAGE_SIZE) {
        ttkernel->set_page(pa, pa);
    }

    ttkernel->activate();
    setup_interrupt();

    g_uart = new pl011_driver(0x09000000ull + KA_BEGIN);
    initialize_filesystem(reinterpret_cast<void*>(pa_memdisk_begin + KA_BEGIN), pa_memdisk_end - pa_memdisk_begin);
    initialize_process_subsystem();
    initialize_timer();
    initialize_logging();

    create_process("/app/init");

    // kallocator->enable_logging();

    schedule();
    
    while(1);
}

}

// at exit
extern "C" int atexit(void (*)()) { return 0; }


extern "C" void kmain(wwos::uint64_t pa_memdisk_begin, wwos::uint64_t pa_memdisk_end) {
    wwos::kernel::main(pa_memdisk_begin, pa_memdisk_end); 
}
