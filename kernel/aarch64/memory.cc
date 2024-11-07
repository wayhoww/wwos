#include "memory.h"
#include "wwos/stdint.h"

namespace wwos::kernel {

// https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Long-descriptor-translation-table-format/Long-descriptor-translation-table-format-descriptors
struct table_descriptor {
    union {
        struct {
            uint64_t valid: 1;
            uint64_t type: 1;
            uint16_t ignored: 10;
            uint64_t next_level_table_addr: 28;
            uint64_t unk_sbzp: 12;
            uint64_t ignored2: 7;
            uint64_t pxn_table: 1;
            uint64_t xn_table: 1;
            uint64_t ap_table: 2;
            uint64_t ns_table: 1;
        }  __attribute__((packed));
        uint64_t raw = 0;
    };
};
static_assert(sizeof(table_descriptor) == 8, "table_descriptor size is not 8 bytes");

// https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Long-descriptor-translation-table-format/Long-descriptor-translation-table-format-descriptors
struct page_descriptor {
    union {
        struct {
            uint64_t valid: 1;
            uint64_t type: 1;
            uint64_t index: 3;           // 2:4
            uint64_t ns: 1;              // 5
            uint64_t ap: 2;              // 6:7
            uint64_t sh: 2;              // 8:9
            uint64_t af: 1;              // 10
            uint64_t ng: 1;              // 11
            uint64_t addr: 28;
            uint64_t unk_sbzp: 12;
            uint64_t contig: 1;          // 52
            uint64_t pxn: 1;             // 53
            uint64_t uxn: 1;             // 54
            uint64_t preserved: 4;       // 55:58
            uint64_t _padding2: 5;       // 59:63
        } __attribute__((packed)) ;
        uint64_t raw = 0;
    };
};
static_assert(sizeof(page_descriptor) == 8, "page_descriptor size is not 8 bytes");


template <translation_table_regime regime>
void translation_table<regime>::set_page(uint64_t va, uint64_t pa, uint64_t level){
    auto index = (va >> LEVEL_OFFSET[level]) & (0x1ff);
    if(level == MAXIMUM_LEVEL) {
        auto& page = reinterpret_cast<page_descriptor&>(items[index]);
        page.valid = 1;
        page.type = 1; // not sure, but looks like different for l1&l2 vs l3
        page.addr = pa >> 12;
        page.af = 1;

        if constexpr (regime == translation_table_regime::USER) {
            page.ap = 0b01; // 0b11: read/write
        }
    } else {
        if((items[index] & 0x1) == 0) {
            auto allocated = new (std::align_val_t(4096)) translation_table;
            uint64_t pa_allocated = uint64_t(allocated) - KA_BEGIN;
            auto& table = reinterpret_cast<table_descriptor&>(items[index]);
            table.valid = 1;
            table.type = 1;
            table.next_level_table_addr = pa_allocated >> 12;
        }
        auto& table = reinterpret_cast<table_descriptor&>(items[index]);
        auto next_level_table = reinterpret_cast<translation_table*>((table.next_level_table_addr << 12) + KA_BEGIN);
        next_level_table->set_page(va, pa, level + 1);
    }
}

template <translation_table_regime regime>
void translation_table<regime>::collect_pages(vector<pair<uint64_t, uint64_t>>& pages, uint64_t goffset, uint64_t level) {
    for(size_t i = 0; i < 512; i++) {
        if((items[i] & 0x1) == 0) {
            continue;
        }
        if(level == MAXIMUM_LEVEL) {
            auto& page = reinterpret_cast<page_descriptor&>(items[i]);
            uint64_t physical_address = page.addr << 12ull;

            pages.push_back({goffset + (i << LEVEL_OFFSET[level]), physical_address});
        } else {
            auto& table = reinterpret_cast<table_descriptor&>(items[i]);
            auto next_level_table = reinterpret_cast<translation_table*>((table.next_level_table_addr << 12) + KA_BEGIN);
            next_level_table->collect_pages(pages, goffset + (i << LEVEL_OFFSET[level]), level + 1);
        }
    }
}

template <translation_table_regime regime>
vector<pair<uint64_t, uint64_t>> translation_table<regime>::get_all_pages() {
    vector<pair<uint64_t, uint64_t>> pages;
    collect_pages(pages);
    return pages;
}

template <translation_table_regime regime>
void translation_table<regime>::activate() {
    auto pa = reinterpret_cast<uint64_t>(items) - KA_BEGIN;

    if constexpr (regime == translation_table_regime::KERNEL) {
        asm volatile(R"(
            MOV      x0, %0
            MSR      TTBR1_EL1, x0

            TLBI     VMALLE1
            DSB      SY
            ISB
        )" : : "r" (pa): "x0");
    } else {
        asm volatile(R"(
            MOV      x0, %0
            MSR      TTBR0_EL1, x0

            TLBI     VMALLE1
            DSB      SY
            ISB
        )" : : "r" (pa): "x0");
    }
}

template class translation_table<translation_table_regime::KERNEL>;
template class translation_table<translation_table_regime::USER>;

}