#include "wwos/stdint.h"
#include "wwos/stdio.h"


extern wwos::uint64_t kernel_blob_begin_mark;
extern wwos::uint64_t loader_memory_begin_mark;
extern wwos::uint64_t loader_memory_end_mark;
extern wwos::uint64_t memdisk_blob_begin_mark;
extern wwos::uint64_t memdisk_blob_end_mark;
extern "C" void loader_main();
extern wwos::uint64_t _start;

extern "C" void wwos_aarch64_handle_exception() {
    wwos::println("exception detected");
}

namespace wwos::kernel {
    void kputchar(char c) {
        volatile char* uart = reinterpret_cast<char*>(0x09000000);
        *uart = c;
    }
}

namespace wwos::boot {


static_assert(KA_BEGIN % (1 << 30) == 0, "KA_BEGIN must be aligned to 1GB");


void setup_translation_table() {
    // assume: we are at el1
    // use only L1

    static uint64_t descriptors[512] __attribute__((aligned(4096)));

    for(uint64_t i = 0; i < 4; i++) {
        // reference: https://developer.arm.com/documentation/102416/0100/Single-level-table-at-EL3/Understand-how-an-entry-is-formed
        // https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Long-descriptor-translation-table-format/Long-descriptor-translation-table-format-descriptors

        descriptors[i] = i << 30 | 0x1 | (1 << 10);
    }

    asm volatile(R"(
        MOV      x0, %0
        MSR      TTBR0_EL1, x0
        MSR      TTBR1_EL1, x0

        MSR      MAIR_EL1, xzr
        MOV      x0, #0x19              // T0SZ=0b011001 Limits VA space to 39 bits, translation starts @ l1
        MOV      x1, #(0x19 << 16)      // T1SZ=0b011001 Limits VA space to 39 bits, translation starts @ l1
        ORR      x0, x0, x1
        MSR      TCR_EL1, x0

        TLBI     VMALLE1
        DSB      SY
        ISB

        MOV      x0, #(1 << 0)                      // M=1           Enable the stage 1 MMU
        ORR      x0, x0, #(1 << 2)                  // C=1           Enable data and unified caches
        ORR      x0, x0, #(1 << 12)                 // I=1           Enable instruction fetches to allocate into unified caches
                                                    // A=0           Strict alignment checking disabled
                                                    // SA=0          Stack alignment checking disabled
                                                    // WXN=0         Write permission does not imply XN
                                                    // EE=0          EL3 data accesses are little endian
        MSR      SCTLR_EL1, x0
        ISB
    )": : "r"(&descriptors));
}

void main() {
    asm volatile(R"(
        ADR x0, aarch64_exception_vector_table;
        MSR VBAR_EL1, x0;
    )");

    setup_translation_table();

    uint64_t kernel_space_begin = KA_BEGIN;
    uint64_t kernel_blob_begin = reinterpret_cast<uint64_t>(&kernel_blob_begin_mark) + kernel_space_begin;

    uint64_t memdisk_blob_begin = (uint64_t) &memdisk_blob_begin_mark;
    uint64_t memdisk_blob_end = (uint64_t) &memdisk_blob_end_mark;

    // jmp to kernel
    asm volatile(
        R"(
            MOV      x0, %0
            MOV      x1, %1
            MOV      x2, %2
            BR       x2
        )"
        :
        :
        "r"(memdisk_blob_begin),
        "r"(memdisk_blob_end),
        "r"(kernel_blob_begin)
    );
}

}

extern "C" void loader_main() {
    wwos::boot::main();
}
