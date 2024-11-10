#include "wwos/format.h"
#include "wwos/stdio.h"
#include "wwos/syscall.h"


int main() {
    wwos::set_priority(10);
    for(int i = 0; i < 10000; i++) {
        wwos::printf("this is demo2 - {}\n", i);
    }
    wwos::println("demo2 done");
    return 0;
}