#include "drivers/pl011.h"

#include "logging.h"
#include "global.h"
#include "wwos/stdint.h"


namespace wwos::kernel {;

    void kputchar(char c) {
        if(g_uart) {
            g_uart->write(c);
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