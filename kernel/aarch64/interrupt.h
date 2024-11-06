#ifndef _WWOS_KERNEL_AARCH64_INTERRUPT_H
#define _WWOS_KERNEL_AARCH64_INTERRUPT_H

#include "wwos/stdint.h"
namespace wwos::kernel {

struct process_state {
    uint64_t spsr = 0;
    uint64_t registers[31] = {0};
};

void setup_interrupt();

[[noreturn]] void eret_to_unprivileged(uint64_t addr, uint64_t sp_user, uint64_t sp_kernel, process_state state, bool withret = false, uint64_t ret = 0);

void initialize_timer();
void set_timeout_interrupt(uint64_t microsecond);
void enable_irq();
void disable_irq();

}

#endif