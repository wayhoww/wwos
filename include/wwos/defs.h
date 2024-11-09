#ifndef _WWOS_DEFS_H
#define _WWOS_DEFS_H

#include "wwos/stdint.h"
namespace wwos {
    // also edit applications/linker.ld.tmpl

    constexpr uint64_t USERSPACE_TEXT          __attribute__((unused)) = 0x200000;
    constexpr uint64_t USERSPACE_STACK_BOTTOM  __attribute__((unused)) = 0x200000000;
    constexpr uint64_t USERSPACE_STACK_TOP     __attribute__((unused)) = 0x240000000;
    constexpr uint64_t USERSPACE_HEAP          __attribute__((unused)) = 0x400000000;  // 16 GB
    constexpr uint64_t USERSPACE_HEAP_END      __attribute__((unused)) = 0x2000000000; // 128 GB
    constexpr uint64_t USERSPACE_END           __attribute__((unused)) = 0x2000000000; // 128 GB

    constexpr uint64_t KERNEL_STACK_SIZE       __attribute__((unused)) = 0x1 << 20; // 1 MB
}

#endif