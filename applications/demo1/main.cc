#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/syscall.h"


int main() {
    wwos::set_priority(1000);
    for(int i = 0; i < 10000; i++) {
        wwos::printf("this is demo1 - {}\n", i);
    }
    wwos::println("demo1 done");
    return 0;
}