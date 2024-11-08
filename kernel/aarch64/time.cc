#include "time.h"
#include "wwos/format.h"

namespace wwos::kernel {
    wwos::uint64_t get_cpu_time() {
        uint64_t physical_clock = 0;
        uint64_t physical_freq = 0;

        asm volatile("MRS %0, CNTPCT_EL0" : "=r"(physical_clock));
        asm volatile("MRS %0, CNTFRQ_EL0" : "=r"(physical_freq));
        
        uint64_t physical_clock_div = physical_clock / physical_freq;
        uint64_t physical_clock_mod = physical_clock % physical_freq;

        uint64_t physical_time = physical_clock_div * 1000000 + physical_clock_mod * 1000000 / physical_freq;

        return physical_time;
    }
}