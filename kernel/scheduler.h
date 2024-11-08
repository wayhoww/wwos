#ifndef _WWOS_KERNEL_SCHEDULER_H
#define _WWOS_KERNEL_SCHEDULER_H

#include "semaphore.h"
#include "wwos/vector.h"
#include "wwos/avl.h"

#include "process.h"


namespace wwos::kernel {

    class task_info_ptr {
    public:
        task_info_ptr(task_info* task): task(task) {}
        task_info* operator->() {return task; }
        bool operator<(const task_info_ptr& other) const;
        void destroy();

    protected:
        task_info* task;
    };

    class scheduler {
    public:
        scheduler() {}
        scheduler(const scheduler&) = delete;
        scheduler(scheduler&&) = delete;
        scheduler& operator=(const scheduler&) = delete;
        scheduler& operator=(scheduler&&) = delete;
        ~scheduler() = default;

        void add_task(task_info* task);
        void replace_task(task_info* task_to_delete, task_info* task_to_add);
        task_info* schedule();
        void remove_task(task_info* task);
        task_info* get_executing_task();

        

    private:
        uint64_t physical_time_start = 0;
        task_info* executing_task = nullptr;
        
        avl_tree<task_info_ptr> active_tasks;
    };
}

#endif