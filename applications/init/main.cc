#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/syscall.h"


int main() {
    wwos::uint32_t pid;

    pid = wwos::fork();
    if(pid == 0) {
        wwos::exec("/app/tty");
    }

    while(true);

    return 0;
}