#include "wwos/assert.h"
#include "wwos/avl.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>


int main() {
    wwos::avl_tree<int> tree;
    srand(time(nullptr));

    int N = 10;

    while(N < 10000000) {
        auto time_begin = std::chrono::high_resolution_clock::now();

        std::vector<int> v;

        for(int i = 0; i < N; i++) {
            v.push_back(rand());
            tree.insert(v.back());
        }

        std::cout << "tree.height() = " << tree.height() << std::endl;

        std::sort(v.begin(), v.end());

        int counter = 0;
        while(!tree.empty()) {
            auto smallest = tree.smallest();
            wwassert(counter < v.size() && smallest->data == v[counter++], "wrong order");
            tree.remove(smallest);
        }
        wwassert(counter == v.size(), "wrong size");

        auto time_end = std::chrono::high_resolution_clock::now(); 
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_begin);

        std::cout << "N = " << N << ", duration = " << duration.count() * 1.0 / 1000000 << " s" << std::endl;

        N *= 10;
    }

    std::cout << "test passed" << std::endl;
}