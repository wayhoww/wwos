#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/syscall.h"


int main() {
    wwos::println("Please enter a priority (10-1000):");
    auto str = wwos::getline().strip();
    wwos::int32_t out;
    auto succ = wwos::stoi(str, out);
    if(!succ) {
        wwos::println("Failed to parse number");
        return 1;
    }
    if(out < 10 || out > 1000) {
        wwos::println("Invalid priority, should be between 10 and 1000");
        return 1;
    }
    wwos::set_priority(out);
    for(int i = 0; i < 500; i++) {
        wwos::printf("this is demo for priority - {}\n", i);
    }
    wwos::println("demo done");
    return 0;
}