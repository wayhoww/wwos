#include "wwos/format.h"
#include "wwos/stdio.h"


int main() {
    wwos::println("What's your name?");
    auto name = wwos::getline();
    name = name.strip();
    wwos::printf("Hello, {}!\n", name);
    return 0;
}