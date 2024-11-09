#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/syscall.h"


void printhex_memoryless(wwos::uint64_t x) {
    // putchar directly
    for(int i = 60; i >= 0; i-=4) {
        int v = (x >> i) & 0xf;
        if(v < 10) {
            wwos::putchar('0' + v);
        } else {
            wwos::putchar('a' + v - 10);
        }
    }
}



int main() {
    
    wwos::uint32_t pid;

    pid = wwos::fork();
    if(pid == 0) {
        wwos::exec("/app/tty");
    }

    pid = wwos::fork();
    if(pid == 0) {
        wwos::exec("/app/shell");
    }

    while(true);

    return 0;
}