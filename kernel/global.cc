#include "drivers/pl011.h"
#include "filesystem.h"
#include "memory.h"
#include "wwos/alloc.h"
#include "wwos/stdint.h"
#include "global.h"

namespace wwos::kernel {

wwos::allocator* kallocator = nullptr;
physical_memory_page_allocator* pallocator = nullptr;
translation_table_kernel* ttkernel = nullptr;

pl011_driver* g_uart = nullptr;

shared_file_node* kernel_logging_sfn = nullptr;

}


void* operator new(wwos::size_t size) {
    auto mem = wwos::kernel::kallocator->allocate(size, 8);
    wwassert(mem, "Failed to allocate memory");
    return mem;
}

void* operator new[](wwos::size_t size) {
    auto mem = wwos::kernel::kallocator->allocate(size, 8);
    wwassert(mem, "Failed to allocate memory");
    return mem;
}

void operator delete(void* address) {
    wwos::kernel::kallocator->deallocate(address);
}

void operator delete[](void* address) {
    wwos::kernel::kallocator->deallocate(address);
}

void* operator new(wwos::size_t size, std::align_val_t align) {
    auto mem = wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
    wwassert(mem, "Failed to allocate memory");
    return mem;
}

void* operator new[](wwos::size_t size, std::align_val_t align) {
    auto mem = wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
    wwassert(mem, "Failed to allocate memory");
    return mem;
}

void operator delete(void* address, std::align_val_t align) {
    wwos::kernel::kallocator->deallocate(address);
}

void operator delete[](void* address, std::align_val_t align) {
    wwos::kernel::kallocator->deallocate(address);
}


void* operator new(wwos::size_t size, wwos::size_t align) {
    auto mem = wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
    wwassert(mem, "Failed to allocate memory");
    return mem;
}

void* operator new[](wwos::size_t size, wwos::size_t align) {
    auto mem =  wwos::kernel::kallocator->allocate(size, static_cast<wwos::size_t>(align));
    wwassert(mem, "Failed to allocate memory");
    return mem;
}

void operator delete(void* address, wwos::size_t align) {
    wwos::kernel::kallocator->deallocate(address);
}

void operator delete[](void* address, wwos::size_t align) {
    wwos::kernel::kallocator->deallocate(address);
}
