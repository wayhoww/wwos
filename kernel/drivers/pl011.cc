#include "pl011.h"


namespace wwos::kernel {

    constexpr size_t UARTFR = 0x18;
    constexpr size_t pl011_FR_RXFF = 1 << 6;

    pl011_driver::pl011_driver(size_t base) : base(base) {}

    void pl011_driver::initialize() {}

    bool pl011_driver::readable() {
        return *reinterpret_cast<volatile uint32_t*>(base + UARTFR) & pl011_FR_RXFF;
    }

    int32_t pl011_driver::read() {
        if(readable()) {
            return *reinterpret_cast<volatile uint32_t*>(base);
        } else {
            return -1;
        }
    }

    void pl011_driver::write(int32_t value) {
        *reinterpret_cast<volatile uint32_t*>(base) = value;
    }

}