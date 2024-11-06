#ifndef _WWOS_KERNEL_DRIVERS_pl011_H
#define _WWOS_KERNEL_DRIVERS_pl011_H

#include "wwos/stdint.h"
namespace wwos::kernel {
    class pl011_driver {
    public:
        pl011_driver(size_t base);
        void initialize();

        bool readable();
        int32_t read();
        void write(int32_t value);

    protected:
        size_t base;
    };
}

#endif