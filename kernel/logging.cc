#include "drivers/pl011.h"

#include "logging.h"
#include "global.h"

#include "raspi4b/io.h"

#include "wwos/stdint.h"


namespace wwos::kernel {;
    void kputchar(char c) {
        if(g_uart) {
#ifdef WWOS_BOARD_RASPI4B
            if(c == '\n') {
                uart_writeByteBlockingActual('\r');
            }
            uart_writeByteBlockingActual(c);
#else
            g_uart->write(c);
#endif
            
        } else {
// #ifdef PA_UART_LOGGING
//             auto uart_addr = reinterpret_cast<volatile uint32_t*>(PA_UART_LOGGING);
//             *uart_addr = c;
// #endif
        }
    }

    int32_t kgetchar() {
        if(g_uart) {
            return g_uart->read();
        }
        return -1;
    }

    void kputchars(const char* s, size_t n) {
        if(s + n >= reinterpret_cast<char*>(KA_BEGIN)) {
            return;
        }

        for(size_t i = 0; i < n; i++) {
            kputchar(s[i]);
        }
    }
}