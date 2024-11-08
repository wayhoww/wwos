#ifndef _WWOS_KERNEL_AARCH64_TIME_H
#define _WWOS_KERNEL_AARCH64_TIME_H

#include "wwos/stdint.h"

namespace wwos::kernel {
    wwos::uint64_t get_cpu_time();
}

#endif