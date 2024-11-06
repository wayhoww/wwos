#include "wwos/assert.h"
#include "memory.h"

namespace wwos::kernel {

physical_memory_page_allocator::physical_memory_page_allocator(size_t begin, size_t size, size_t page_size): page_size(page_size) {
    if (begin % page_size != 0 || size % page_size != 0) {
        wwassert(false, "misaligned memory");
    }
    freelist.push_back(physical_page_node{begin, size});
}

size_t physical_memory_page_allocator::alloc() {
    if (freelist.size() == 0) {
        return 0;
    }
    auto& node = freelist.back();
    size_t addr = node.addr;
    node.size -= page_size;
    node.addr += page_size;

    if (node.size == 0) {
        freelist.pop_back();
    }

    return addr;
}

bool physical_memory_page_allocator::alloc_specific_page(size_t addr) {
    size_t i = 0;
    for (auto& node : freelist) {
        if (node.addr > addr) {
            break;
        }
        i += 1;
    }

    if (i < freelist.size() && freelist[i].addr + freelist[i].size >= addr + page_size) {
        if (freelist[i].size == page_size) {
            freelist.erase(freelist.begin() + i);
            return true;
        } else {
            if (freelist[i].addr == addr) {
                freelist[i].addr += page_size;
                freelist[i].size -= page_size;
                return true;
            } else if (freelist[i].addr + freelist[i].size == addr + page_size) {
                freelist[i].size -= page_size;
                return true;
            } else {
                // split into two
                size_t chunk_size = freelist[i].size;
                freelist[i].size = addr - freelist[i].addr;
                freelist.insert(freelist.begin() + i + 1, physical_page_node{addr + page_size, chunk_size - freelist[i].size - page_size});
                return true;
            }
        }
    } else {
        return false;
    }
}

void physical_memory_page_allocator::free(size_t addr) {
    size_t i = 0;
    for (auto& node : freelist) {
        if (node.addr > addr) {
            break;
        }
        i += 1;
    }

    bool left_contiguous = i > 0 && freelist[i - 1].addr + freelist[i - 1].size == addr;
    bool right_contiguous = i < freelist.size() && addr + page_size == freelist[i].addr;

    if (left_contiguous && right_contiguous) {
        freelist[i - 1].size += freelist[i].size + page_size;
        freelist.erase(freelist.begin() + i);
    } else if (left_contiguous) {
        freelist[i - 1].size += page_size;
    } else if (right_contiguous) {
        freelist[i].addr -= page_size;
        freelist[i].size += page_size;
    } else {
        freelist.insert(freelist.begin() + i, physical_page_node{addr, page_size});
    }
}

}
