#ifndef _WWOS_KERNEL_LOGGING_H
#define _WWOS_KERNEL_LOGGING_H

#include "wwos/stdint.h"

namespace wwos::kernel {
    void kputchar(char c);
    int32_t kgetchar();
}

#endif