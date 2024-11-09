#ifndef _WWOS_KERNEL_MEMORY_H
#define _WWOS_KERNEL_MEMORY_H

#include "wwos/stdint.h"
#include "wwos/vector.h"

namespace wwos::kernel {

struct physical_page_node {
    size_t addr;
    size_t size;
};


class physical_memory_page_allocator {
public:
    physical_memory_page_allocator(size_t begin, size_t size, size_t page_size);

    size_t alloc(size_t n = 1);
    bool alloc_specific_page(size_t addr, size_t n = 1);
    void free(size_t addr);

private:
   vector<physical_page_node> freelist;
   size_t page_size;
};

}

#endif