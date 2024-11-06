#ifndef _WWOS_KERNEL_DRIVER_GIC2_H
#define _WWOS_KERNEL_DRIVER_GIC2_H

#include "wwos/stdint.h"
namespace wwos::kernel {

// https://lowenware.com/blog/aarch64-gic-and-timer-interrupt/
class gic02_driver {
public:
    gic02_driver(size_t gicd_base, size_t gicc_base);

    void initialize();

    void enable(size_t interrupt);

    void disable(size_t interrupt);

    void clear(size_t interrupt);

    void set_core(size_t interrupt, size_t core);

    void set_priority(size_t interrupt, size_t priority);

    void set_config(size_t interrupt, size_t config);

    // GICC_IAR & GICC_EOIR contains CPUID && interrupt ID
    // currently support only CPU 0

    uint32_t get_interrupt_id();

    void finish_interrupt(uint32_t interrupt_id);

protected:
    size_t gicd_base;
    size_t gicc_base;
};

}

#endif