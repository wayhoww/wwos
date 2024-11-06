#include "pl011.h"


//    // 0x018
//     volatile uint32_t* addr2 = reinterpret_cast<volatile uint32_t*>(KA_BEGIN + 0x09000018);
//     while(true) {
//         uint32_t flag = *addr2;
//         if(flag >> 6 & 1) {
//             println("full");

//             uint32_t data = *reinterpret_cast<volatile uint32_t*>(KA_BEGIN + 0x09000000);
//             printf("data: {}\n", data);
//         } 

//     }


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