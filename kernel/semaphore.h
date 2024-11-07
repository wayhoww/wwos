#ifndef _WWOS_KERNEL_SEMAPHORE_H
#define _WWOS_KERNEL_SEMAPHORE_H

#include "process.h"

#include "wwos/stdint.h"
#include "wwos/vector.h"

namespace wwos::kernel {

    class semaphore {
    public:
        semaphore(int64_t count): count(count) {}
        semaphore(const semaphore&) = delete;
        semaphore(semaphore&&) = delete;
        semaphore& operator=(const semaphore&) = delete;
        semaphore& operator=(semaphore&&) = delete;
        ~semaphore() = default;

        void wait();
        void signal();
    private:
        int64_t count = 0;
        wwos::vector<task_info*> waiting_tasks;
    };

}

#endif