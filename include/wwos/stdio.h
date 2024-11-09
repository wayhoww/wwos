#ifndef _WWOS_STDIO_H
#define _WWOS_STDIO_H

#ifdef WWOS_HOST
#include <cstdio>
#endif

#include "wwos/string.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"

#ifdef WWOS_KERNEL
    namespace wwos::kernel { extern void kputchar(char c); }
#endif

namespace wwos {

    extern wwos::int64_t fd_stdin;
    extern wwos::int64_t fd_stdout;

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
        #ifdef WWOS_KERNEL
        kernel::kputchar(c);
        #elif defined(WWOS_HOST)
        std::putchar(c);
        #else
        // syscall(syscall_id::PUTCHAR, c);
        wwos::write(fd_stdout, (uint8_t*)&c, 1);
        #endif
    }

    inline int32_t getchar() {
        int32_t c;
        do {
            uint64_t size = wwos::read(fd_stdin, (uint8_t*)&c, 1);
            wwassert(size >= 0, "Failed to read from stdin");
            if(size == 1) return c;
        } while(true);
    }

    inline void print(string_view s) {
        for (size_t i = 0; i < s.size(); i++) {
            putchar(s[i]);
        }
    }

    inline void println(string_view s) {
        print(s);
        putchar('\n');
    }

    template <typename T>
    void printhex(T value) {
        constexpr char hex[] = "0123456789ABCDEF";
        print("0x");
        for (size_t i = 0; i < sizeof(T) * 2; i++) {
            putchar(hex[(value >> (sizeof(T) * 8 - 4 * (i + 1))) & 0xF]);
        }
    }
}

#endif