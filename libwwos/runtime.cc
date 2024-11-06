#include "wwos/alloc.h"
#include "wwos/assert.h"
#include "wwos/defs.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/syscall.h"

int main();


wwos::size_t PAGE_SIZE = 4096;

class paged_allocator {
public:
    paged_allocator(): heap_size(PAGE_SIZE), alloc(wwos::allocator(wwos::USERSPACE_HEAP, PAGE_SIZE)) {
        // alloc.enable_logging();
    }

    void* allocate(wwos::size_t size, wwos::size_t align) {
        void* ptr =  alloc.allocate(size, align);
        if(ptr) return ptr;

        auto required_size = size + align + 16;
        auto pages = (required_size + PAGE_SIZE - 1) / PAGE_SIZE;
        for(wwos::size_t i = 0; i < pages; i++) {
            wwassert(wwos::allocate_page(wwos::USERSPACE_HEAP + heap_size), "Failed to allocate page");
            heap_size += PAGE_SIZE;
        }
        alloc.extend(wwos::USERSPACE_HEAP + heap_size);
    
        return alloc.allocate(size, align);
    }

    void deallocate(void* address) {
        alloc.deallocate(address);
    }

private:
    wwos::size_t heap_size = 0;
    wwos::allocator alloc;
};

paged_allocator* uallocator = nullptr;

extern "C" void _wwos_runtime_entry(int* argc, char*** argv) {
    bool succ = wwos::allocate_page(wwos::USERSPACE_HEAP);
    wwassert(succ, "Failed to allocate page");
    wwos::println("Allocated page");

    static paged_allocator suallocator;
    uallocator = &suallocator;

    main();
}

void* operator new(wwos::size_t size) {
    void* out = uallocator->allocate(size, 8);
    wwassert(out, "Failed to allocate memory");
    return out;
}

void* operator new[](wwos::size_t size) {
    void* out = uallocator->allocate(size, 8);
    wwassert(out, "Failed to allocate memory");
    return out;
}

void operator delete(void* address) {
    uallocator->deallocate(address);
}

void operator delete[](void* address) {
    uallocator->deallocate(address);
}

void* operator new(wwos::size_t size, std::align_val_t align) {
    void* out = uallocator->allocate(size, static_cast<wwos::size_t>(align));
    wwassert(out, "Failed to allocate memory");
    return out;
}

void* operator new[](wwos::size_t size, std::align_val_t align) {
    void* out = uallocator->allocate(size, static_cast<wwos::size_t>(align));
    wwassert(out, "Failed to allocate memory");
    return out;
}

void operator delete(void* address, std::align_val_t align) {
    uallocator->deallocate(address);
}

void operator delete[](void* address, std::align_val_t align) {
    uallocator->deallocate(address);
}
