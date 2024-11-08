#include "wwos/algorithm.h"
#include "wwos/assert.h"
#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/alloc.h"


namespace wwos {
struct chunk_header {
    size_t size;
    chunk_header* next;
};


allocator::allocator(size_t address, size_t size) {
    if (size < sizeof(chunk_header)) {
        wwassert(false, "invalid size");
    }

    head = (chunk_header* )address;
    chunk_header* next = (chunk_header* )(address + sizeof(chunk_header));

    head->size = 0;
    head->next = next;

    next->size = size - sizeof(chunk_header) * 2;
    next->next = nullptr;

    begin = address;
    this->size = size;
}

void* allocator::allocate(size_t size, size_t align) {
    align = align_up<size_t>(align, 8);
    size = align_up<size_t>(size, 8);

    if (logging) {
        print("Current LOC: allocator::allocate, begin  size=");
        printhex(size);
        print(" align=");
        printhex(align);
        println("");
        dump();
    }

    auto [prev, current] = find_chunk(size, align);

    if (current == nullptr) {
        return nullptr;
    }
    chunk_header* chunk = current;
    size_t start = reinterpret_cast<size_t>(current) + sizeof(chunk_header);
    size_t aligned_start = (start + align - 1) & ~(align - 1);
    size_t end = aligned_start + size;

    if (chunk->size > end - start + sizeof(chunk_header)) {
        chunk_header* new_chunk = (chunk_header* )(end);
        new_chunk->size = chunk->size - (end - start) - sizeof(chunk_header);
        new_chunk->next = chunk->next;

        prev->next = new_chunk;
    } else {
        prev->next = chunk->next;
    }

    chunk_header* new_chunk = (chunk_header* )(aligned_start - sizeof(chunk_header));
    new_chunk->size = end - start;
    new_chunk->next = current;

    if (logging) {
        print("Current LOC: allocator::allocate, end  return=");
        printhex(aligned_start);
        print("   chunk=");
        printhex(reinterpret_cast<size_t>(new_chunk));
        print("   size=");
        printhex(new_chunk->size);
        print("   next=");
        printhex(reinterpret_cast<size_t>(new_chunk->next));
        print("   start=");
        printhex(start);
        print("   end=");
        printhex(end);
        println("");
        dump();
    }

    return (void* )aligned_start;
}

void allocator::deallocate(void* address) {
    // println("Current LOC: allocator::deallocate");
    if (logging) {
        print("Current LOC: allocator::deallocate, start   address=");
        printhex(reinterpret_cast<size_t>(address));
        println("");
        dump();
    }

    if (address == nullptr) {
        return;
    }
    chunk_header* p_chunk = (chunk_header*)(reinterpret_cast<size_t>(address) - sizeof(chunk_header));
    chunk_header* chunk = p_chunk;

    if (logging) {
        print("Current LOC: allocator::deallocate, chunk=");
        printhex(reinterpret_cast<size_t>(chunk));
        print("   size=");
        printhex(chunk->size);
        print("   next=");
        printhex(reinterpret_cast<size_t>(chunk->next));
        println("");
    }

    chunk_header* new_p_chunk = chunk->next;

    chunk_header* prev = head;
    chunk_header* current = head->next;

    while (prev != nullptr) {
        if (prev < new_p_chunk && (new_p_chunk < current || current == nullptr)) {
            size_t end_of_prev = reinterpret_cast<size_t>(prev) + sizeof(chunk_header) + prev->size;
            // wwmark("msg");
            size_t end_of_new_p_chunk = reinterpret_cast<size_t>(new_p_chunk) + sizeof(chunk_header) + chunk->size;
            // wwmark("msg");
            if (end_of_prev > reinterpret_cast<size_t>(new_p_chunk) || (current != nullptr && end_of_new_p_chunk > reinterpret_cast<size_t>(current))) {
                wwassert(false, "memory corrupted");
            }

            bool connected_with_prev = (reinterpret_cast<size_t>(prev) + sizeof(chunk_header) + prev->size == reinterpret_cast<size_t>(new_p_chunk)) && (prev != head);
            bool connected_with_current = (reinterpret_cast<size_t>(new_p_chunk) + sizeof(chunk_header) + chunk->size == reinterpret_cast<size_t>(current)) && (current != nullptr);

            wwassert(!(current == nullptr && connected_with_current), "memory corrupted");

            if (connected_with_prev && connected_with_current) {
                prev->size += 2 * sizeof(chunk_header) + chunk->size + current->size;
                prev->next = current->next;
            } else if (connected_with_prev) {
                prev->size += sizeof(chunk_header) + chunk->size;
                prev->next = current;
            } else if (connected_with_current) {
                new_p_chunk->size += sizeof(chunk_header) + current->size;
                new_p_chunk->next = current->next;
                prev->next = new_p_chunk;
            } else {
                new_p_chunk->next = current;
                new_p_chunk->size = chunk->size;
                prev->next = new_p_chunk;
            }

            if (logging) {
                println("Current LOC: allocator::deallocate, return");
                dump();
            }
            return;
        }

        prev = current;
        current = current->next;
    }
    wwassert(false, "memory corrupted");
}

void allocator::extend(size_t new_end) {
    chunk_header* p_tail = get_tail();
    size_t a_tail = reinterpret_cast<size_t>(p_tail);
    chunk_header* tail = p_tail;

    if (a_tail + tail->size + sizeof(chunk_header) == begin + size) {
        tail->size += new_end - (begin + size);
    } else {
        chunk_header* p_new_tail = (chunk_header*)(begin + size);
        chunk_header* new_tail = p_new_tail;
        new_tail->size = new_end - (begin + size) - sizeof(chunk_header);
        new_tail->next = nullptr;

        tail->next = p_new_tail;
    }

    size = new_end - begin;
}

size_t allocator::get_used_address_upperbound() {
    chunk_header* p_tail = get_tail();
    size_t a_tail = reinterpret_cast<size_t>(p_tail);
    chunk_header* tail = p_tail;

    if (a_tail + tail->size + sizeof(chunk_header) == begin + size) {
        return a_tail + sizeof(chunk_header);
    } else {
        return begin + size;
    }
}

chunk_header* allocator::get_tail() {
    chunk_header* prev = head;
    chunk_header* current = head->next;

    while (current != nullptr) {
        prev = current;
        current = current->next;
    }

    return prev;
}

pair<chunk_header*, chunk_header*> allocator::find_chunk(size_t size, size_t align) {
    chunk_header* prev = head;
    chunk_header* current = head->next;

    while (current != nullptr) {
        size_t start = reinterpret_cast<size_t>(current) + sizeof(chunk_header);
        size_t aligned_start = (start + align - 1) & ~(align - 1);
        size_t end = aligned_start + size;

        if (current->size >= end - start) {
            return { prev, current };
        }

        prev = current;
        current = current->next;
    }
    return { nullptr, nullptr };
}

void allocator::dump() {
    if(!logging) {
        return;
    }

    println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> allocator dump >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    auto node = head;
    while(node != nullptr) {
        print("node=");
        printhex(reinterpret_cast<size_t>(node));
        print("   size=");
        printhex(node->size);
        print("   next=");
        printhex(reinterpret_cast<size_t>(node->next));
        print("\n");
        node = node->next;
    }
    println("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< allocator dump <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}
}

extern "C" void* memset(void* dest, int c, wwos::size_t n) {
    wwos::uint8_t* p = reinterpret_cast<wwos::uint8_t*>(dest);
    for(wwos::size_t i = 0; i < n; i++) {
        *p++ = c;
    }
    return dest;
}

extern "C" void* memcpy(void* dest, const void* src, wwos::size_t n) {
    wwos::uint8_t* p_dest = reinterpret_cast<wwos::uint8_t*>(dest);
    const wwos::uint8_t* p_src = reinterpret_cast<const wwos::uint8_t*>(src);
    if (p_dest < p_src) {
        for(wwos::size_t i = 0; i < n; i++) {
            *p_dest++ = *p_src++;
        }
    } else {
        for(wwos::size_t i = 0; i < n; i++) {
            p_dest[n - i - 1] = p_src[n - i - 1];
        }
    }
    return dest;
}

extern "C" void __cxa_pure_virtual() {
    wwassert(false, "pure virtual function called");
}