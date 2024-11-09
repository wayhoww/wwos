#ifndef _WWOS_STDIO_H
#define _WWOS_STDIO_H

#ifdef WWOS_HOST
#include <cstdio>
#endif

#include "wwos/string_view.h"
#include "wwos/syscall.h"

#ifdef WWOS_KERNEL
    namespace wwos::kernel { extern void kputchar(char c); }
#endif

namespace wwos {

#ifdef WWOS_APPLICATION
    extern wwos::int64_t fd_stdin;
    extern wwos::int64_t fd_stdout;
#endif

#ifdef WWOS_KERNEL
    namespace kernel { 
        struct shared_file_node;
        extern shared_file_node* kernel_logging_sfn; 
        extern size_t write_shared_node(void* buffer, shared_file_node* node, size_t offset, size_t size);
    }
#endif

    inline void kputchar(char c) {
#ifdef WWOS_KERNEL
        kernel::kputchar(c);
#elif defined(WWOS_HOST)
        std::putchar(c);
#else
        syscall(syscall_id::PUTCHAR, c);
#endif
    }

    inline int32_t kgetchar() {
        int32_t c = syscall(syscall_id::GETCHAR, 0);
        if(c <= 1) return -1;
        else return c;
    }

    inline void putchar(char c) {
#if defined(WWOS_KERNEL)
#ifdef WWOS_DIRECT_LOGGING
        kputchar(c);
#else
        if(kernel::kernel_logging_sfn == nullptr) {
            kputchar(c);
        } else {
            kernel::write_shared_node(&c, kernel::kernel_logging_sfn, 0, 1);
        }
#endif
#elif defined(WWOS_HOST)
        std::putchar(c);
#else
        wwos::write(fd_stdout, (uint8_t*)&c, 1);
#endif
    }

#ifndef WWOS_KERNEL
    inline int32_t getchar() {
#ifdef WWOS_HOST
        return std::getchar();
#else
        int32_t c;
        do {
            uint64_t size = wwos::read(fd_stdin, (uint8_t*)&c, 1);
            wwassert(size >= 0, "Failed to read from stdin");
            if(size == 1) return c;
        } while(true);
#endif
    }
#endif

    inline void print(string_view s) {
        for (size_t i = 0; i < s.size(); i++) {
            putchar(s[i]);
        }
    }

    inline void println(string_view s) {
        print(s);
        putchar('\n');
    }

    inline void kprint(string_view s) {
        for (size_t i = 0; i < s.size(); i++) {
            kputchar(s[i]);
        }
    }

    inline void kprintln(string_view s) {
        kprint(s);
        kputchar('\n');
    }

    template <typename T>
    void printhex(T value) {
        constexpr char hex[] = "0123456789ABCDEF";
        print("0x");
        for (size_t i = 0; i < sizeof(T) * 2; i++) {
            putchar(hex[(value >> (sizeof(T) * 8 - 4 * (i + 1))) & 0xF]);
        }
    }

    template <typename T>
    void kprinthex(T value) {
        constexpr char hex[] = "0123456789ABCDEF";
        kprint("0x");
        for (size_t i = 0; i < sizeof(T) * 2; i++) {
            kputchar(hex[(value >> (sizeof(T) * 8 - 4 * (i + 1))) & 0xF]);
        }
    }
}

#endif