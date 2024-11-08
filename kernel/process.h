#ifndef _WWOS_KERNEL_PROCESS_H
#define _WWOS_KERNEL_PROCESS_H


#include "aarch64/interrupt.h"
#include "aarch64/memory.h"
#include "wwos/stdint.h"
#include "wwos/string_view.h"

namespace wwos::kernel {

struct process_control {
    uint64_t pc;
    uint64_t ksp;
    uint64_t usp;
    bool has_return_value;
    uint64_t return_value;

    process_state state;
    translation_table_user* tt;

    void set_return_value(uint64_t value) {
        has_return_value = true;
        return_value = value;
    }
};

struct task_info {
    uint64_t vruntime = 0;
    uint16_t priority = 1000;
    uint64_t pid;
    process_control pcb;
};

extern uint64_t current_pid;

void create_process(string_view path, task_info* replacing = nullptr);
void replace_current_task(string_view path);

[[noreturn]] void schedule();

void initialize_process_subsystem();

task_info& get_current_task();

void fork_current_task();

[[noreturn]] void on_timeout();

void kallocate_page(uint64_t va);

}

#endif

