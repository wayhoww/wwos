#include "aarch64/time.h"
#include "wwos/assert.h"
#include "wwos/avl.h"

#include "process.h"
#include "wwos/format.h"
#include "scheduler.h"


namespace wwos::kernel {


bool task_info_ptr::operator<(const task_info_ptr& other) const {
    return task->vruntime < other.task->vruntime;
}

void task_info_ptr::destroy() {
    delete task;
    task = nullptr;
}

void scheduler::add_task(task_info* task) {
    wwassert(task, "task is null");

    if(active_tasks.empty()) {
        task->vruntime = 0;
    } else {
        task->vruntime = max<uint64_t>(active_tasks.smallest()->data->vruntime, 1) - 1;
    }
    active_tasks.insert(task_info_ptr(task));
    if(executing_task == nullptr) {
        schedule();
    }

    wwassert(executing_task != nullptr, "impossible");
}


        // void replace_task(task_info* task_to_delete, task_info* task_to_add);

void scheduler::replace_task(task_info* task_to_delete, task_info* task_to_add) {
    wwassert(task_to_delete, "task_to_delete is null");
    wwassert(task_to_add, "task_to_add is null");

    if(task_to_delete == executing_task) {
        executing_task = task_to_add;
        return;
    }

    auto node = active_tasks.find(task_info_ptr(task_to_delete));
    wwassert(task_to_delete == node->data.operator->(), "task not found");
    task_to_add->vruntime = task_to_delete->vruntime;
    node->data = task_info_ptr(task_to_add);
}

void scheduler::remove_task(task_info* task) {
    if(task == executing_task) {
        executing_task = nullptr;
        schedule();
        return;
    }

    auto node = active_tasks.find(task_info_ptr(task));
    wwassert(task == node->data.operator->(), "task not found");
    active_tasks.remove(node);
}

task_info* scheduler::schedule() {
    auto physical_time = get_cpu_time();

    if(executing_task != nullptr) {
        auto physical_time_spent = physical_time - physical_time_start;
        auto virtual_time_spent = max<uint64_t>(physical_time_spent / executing_task->priority, 1);
        executing_task->vruntime += virtual_time_spent;
        active_tasks.insert(task_info_ptr(executing_task));
        executing_task = nullptr;
    }

    if(!active_tasks.empty()) {
        auto next_task_node = active_tasks.smallest();
        if(next_task_node == nullptr) {
            return nullptr;
        }

        executing_task = next_task_node->data.operator->();
        active_tasks.remove(next_task_node);
        physical_time_start = physical_time;

        wwassert(executing_task, "no task to schedule");
        return executing_task;
    }

    wwassert(false, "no task to schedule");
}

task_info* scheduler::get_executing_task() {
    return executing_task;
}

}
