#include "wwos/assert.h"
#include "wwos/stdio.h"


#ifdef WWOS_KERNEL
namespace wwos::kernel {
    uint64_t get_cpu_time();
}
#endif


namespace wwos {
    void print_int(int x) {
        if(x == 0) {
            putchar('0');
        } else {
            char buf[32];
            int i = 0;
            while(x) {
                buf[i++] = '0' + x % 10;
                x /= 10;
            }
            while(i--) putchar(buf[i]);
        }
    }

    void kprint_int(int x) {
        if(x == 0) {
            kputchar('0');
        } else {
            char buf[32];
            int i = 0;
            while(x) {
                buf[i++] = '0' + x % 10;
                x /= 10;
            }
            while(i--) kputchar(buf[i]);
        }
    }

    void trigger_mark(string_view file, int line, string_view msg) {
        // do not use dynamic memory allocation here
#ifdef WWOS_KERNEL
        print("[");
        print_int(kernel::get_cpu_time());
        print("] ");
#endif
        print("mark at ");
        print(file);
        print(":");
        print_int(line);
        print("    info: ");
        print(msg);
        print("\n");
    }

    void trigger_assert(string_view expr, string_view file, int line, string_view msg) {
        // do not use dynamic memory allocation here
#ifdef WWOS_KERNEL
#define print kprint
#define print_int kprint_int
#endif
        print("assertion failed at ");
        print(file);
        print(":");
        print_int(line);
        print("\n");
        print("    expr: ");
        print(expr);
        print("\n");
        print("    info: ");
        print(msg);
        print("\n");
#ifdef WWOS_KERNEL
#undef print
#undef print_int
#endif
        while (true);
    }
}