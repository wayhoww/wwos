#include "wwos/format.h"
#include "wwos/stdio.h"


int main() {
    wwos::string str("Across the great firewall we can reach every corner in the world.");
    for(int i = 0; i < str.size(); i++) {
        wwos::putchar(str[i]);
        wwos::sleep(100000);
    }
    wwos::println("");
    return 0;
}