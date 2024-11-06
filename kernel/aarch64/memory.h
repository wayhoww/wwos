#ifndef _WWOS_KERNEL_AARCH64_MEMORY_H
#define _WWOS_KERNEL_AARCH64_MEMORY_H

#include "wwos/pair.h"
#include "wwos/stdint.h"
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
    translation_table() = default;
    translation_table(const translation_table&) = delete;
    translation_table& operator=(const translation_table&) = delete;
    // translation_table(translation_table&&);
    // translation_table& operator=(translation_table&&);
    // TODO(ww): destructor?

    constexpr static uint64_t LEVEL_OFFSET[4] = {12 + 27, 12 + 18, 12 + 9, 12 + 0};
    constexpr static uint64_t MAXIMUM_LEVEL = sizeof(LEVEL_OFFSET) / sizeof(LEVEL_OFFSET[0]) - 1;
    constexpr static uint64_t PAGE_SIZE = 1 << LEVEL_OFFSET[MAXIMUM_LEVEL];
    void set_page(uint64_t va, uint64_t pa, uint64_t level = 1);
    vector<pair<uint64_t, uint64_t>> get_all_pages();
    void activate();

private:
    void collect_pages(vector<pair<uint64_t, uint64_t>>& pages, uint64_t goffset = 0, uint64_t level = 1);

    uint64_t items[512] = {0};
} __attribute__((aligned(4096)));

using translation_table_kernel = translation_table<translation_table_regime::KERNEL>;
using translation_table_user = translation_table<translation_table_regime::USER>;

static_assert(sizeof(translation_table_kernel) == 4096, "translation_table size is not 4096 bytes");
static_assert(sizeof(translation_table_user) == 4096, "translation_table size is not 4096 bytes");

}

#endif