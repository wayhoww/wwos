#ifndef _WWOS_ALGORITHM_H
#define _WWOS_ALGORITHM_H

#include "stdint.h"

namespace wwos {

    template<typename T>
    T align_up(T value, T align) {
        return (value + align - 1) & ~(align - 1);
    }

    template<typename T>
    T align_down(T value, T align) {
        return value & ~(align - 1);
    }

    template<typename T>
    T min(T a, T b) {
        return a < b ? a : b;
    }

    template<typename T>
    T max(T a, T b) {
        return a > b ? a : b;
    }

    template<typename T>
    T abs(T a) {
        return a < 0 ? -a : a;
    }

    template<typename T>
    T swap(T a, T b) {
        T tmp = a;
        a = b;
        b = tmp;
    }

    inline uint64_t set_bits(uint64_t val, uint64_t bits, uint64_t begin, uint64_t end) {
        uint64_t mask = (1 << (end - begin)) - 1;
        return (val & ~(mask << begin)) | (bits << begin);
    }

    inline uint64_t get_bits(uint64_t val, uint64_t begin, uint64_t end) {
        return (val >> begin) & ((1 << (end - begin)) - 1);
    }
}

#endif