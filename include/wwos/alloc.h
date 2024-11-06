#ifndef _WWOS_ALLOC_H
#define _WWOS_ALLOC_H

#include "wwos/stdint.h"
#include "wwos/pair.h"

#ifndef WWOS_HOST
// #define MEMORY_DEBUG
#endif



extern "C" void* memset(void* dest, int c, wwos::size_t n);
extern "C" void* memcpy(void* dest, const void* src, wwos::size_t n);

namespace wwos {

struct chunk_header;

class allocator {
public:
    allocator(size_t address, size_t size);
    void *allocate(size_t size, size_t align);
    void deallocate(void *address);
    void extend(size_t new_end);
    size_t get_used_address_upperbound();
    void enable_logging() { logging = true; }
    void disable_logging() { logging = false; }

protected:
    void dump();
    chunk_header *get_tail();
    pair<chunk_header*, chunk_header*> find_chunk(size_t size, size_t align);

protected:
    chunk_header *head;
    size_t begin;
    size_t size;
    bool logging = false;
};

inline void* memset(void* dest, int c, size_t n) {
    return ::memset(dest, c, n);
}

inline void* memcpy(void* dest, const void* src, size_t n) {
    return ::memcpy(dest, src, n);
}

}


#endif