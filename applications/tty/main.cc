#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"


int main() {
    int fd_proc2;
    do {
        fd_proc2 = wwos::open("/proc/2/fifo/stdout", wwos::fd_mode::READONLY);
    } while (fd_proc2 < 0);

    wwos::println("Opened /proc/2/fifo/stdout");

    wwos::uint8_t buffer[512];
    while(true) {
        int ret = wwos::read(fd_proc2, buffer, 512);
        if(ret < 0) {
            wwos::println("Failed to read /proc/2/fifo/stdout");
        }
        if(ret == 0) continue;

        wwos::printf("Read from /proc/2/fifo/stdout, ret = {}\n", ret);
        wwos::print(wwos::string_view((const char*)buffer, ret));
    }
    
    while(true);
    return 0;
}