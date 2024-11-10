#ifndef _WWOS_KERNEL_AARCH64_MEMORY_H
#define _WWOS_KERNEL_AARCH64_MEMORY_H

#include "wwos/alloc.h"
#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/vector.h"


namespace wwos::kernel {

// https://developer.arm.com/documentation/101811/0104/Translation-granule
// start at level 1
// page size = 4KB

enum class translation_table_regime {
    KERNEL = 0,
    USER = 1,
};

template <translation_table_regime regime>
class translation_table {
public:
    translation_table();
    translation_table(const translation_table&) = delete;
    translation_table& operator=(const translation_table&) = delete;
    // translation_table(translation_table&&);
    // translation_table& operator=(translation_table&&);
    // TODO(ww): destructor?
    ~translation_table() {
        destroy_recursively(1, items);
    }

    constexpr static uint64_t LEVEL_OFFSET[4] = {12 + 27, 12 + 18, 12 + 9, 12 + 0};
    constexpr static uint64_t MAXIMUM_LEVEL = sizeof(LEVEL_OFFSET) / sizeof(LEVEL_OFFSET[0]) - 1;
    constexpr static uint64_t PAGE_SIZE = 1 << LEVEL_OFFSET[MAXIMUM_LEVEL];
    void set_page(uint64_t va, uint64_t pa, uint64_t level = 1, uint64_t* level_items = nullptr);
    vector<pair<uint64_t, uint64_t>> get_all_pages();
    void dump();
    void activate();

private:
    translation_table(uint64_t* items): items(items) {}
    void collect_pages(vector<pair<uint64_t, uint64_t>>& pages, uint64_t goffset = 0, uint64_t level = 1, uint64_t* level_items = nullptr);
    void dump_recursively(uint64_t goffset = 0, uint64_t level = 1, uint64_t* level_items = nullptr);
    void destroy_recursively(uint64_t level, uint64_t* level_items);
    uint64_t* items;
};

using translation_table_kernel = translation_table<translation_table_regime::KERNEL>;
using translation_table_user = translation_table<translation_table_regime::USER>;

}

#endif