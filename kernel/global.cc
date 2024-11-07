#include "drivers/pl011.h"
#include "memory.h"
#include "wwos/alloc.h"
#include "wwos/stdint.h"
#include "global.h"

namespace wwos::kernel {

wwos::allocator* kallocator = nullptr;
physical_memory_page_allocator* pallocator = nullptr;
translation_table_kernel* ttkernel = nullptr;

pl011_driver* g_uart = nullptr;

}


void* operator new(wwos::size_t size) {
    return wwos::kernel::kallocator->allocate(size, 8);
}

void* operator new[](wwos::size_t size) {
    return wwos::kernel::kallocator->allocate(size, 8);
}

void operator delete(void* address) {
    wwos::kernel::kallocator->deallocate(address);
}

void operator delete[](void* address) {
    wwos::kernel::kallocator->deallocate(address);
}

void* operator new(wwos::size_t size, std::align_val_t align) {
    return wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
}

void* operator new[](wwos::size_t size, std::align_val_t align) {
    return wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
}

void operator delete(void* address, std::align_val_t align) {
    wwos::kernel::kallocator->deallocate(address);
}

void operator delete[](void* address, std::align_val_t align) {
    wwos::kernel::kallocator->deallocate(address);
}


void* operator new(wwos::size_t size, wwos::size_t align) {
    return wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
}

void* operator new[](wwos::size_t size, wwos::size_t align) {
    return wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
}

void operator delete(void* address, wwos::size_t align) {
    wwos::kernel::kallocator->deallocate(address);
}

void operator delete[](void* address, wwos::size_t align) {
    wwos::kernel::kallocator->deallocate(address);
}
