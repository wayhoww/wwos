#ifndef _WWOS_KERNEL_PROCESS_H
#define _WWOS_KERNEL_PROCESS_H


#include "aarch64/interrupt.h"
#include "aarch64/memory.h"
#include "filesystem.h"
#include "wwos/map.h"
#include "wwos/stdint.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"

namespace wwos::kernel {

struct process_control {
    uint64_t pc;
    uint64_t ksp;
    uint64_t usp;
    bool has_return_value;
    uint64_t return_value;

    process_state state;
    translation_table_user tt;

    void set_return_value(uint64_t value) {
        has_return_value = true;
        return_value = value;
    }
};

struct fd_info {
    shared_file_node* node;
    fd_mode mode;
    uint64_t offset;
};

struct task_info {
    uint64_t vruntime = 0;
    uint16_t priority = 1000;
    uint64_t pid;
    process_control pcb;
    
    uint64_t fd_counter = 0;
    map<uint64_t, fd_info> fds;
};

struct semaphore {
    semaphore(int64_t count): count(count) {}

    wwos::vector<int64_t> waiting_tasks;
    int64_t count = 0;
    bool priviledged = false;
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

void current_task_exit();
void on_data_abort(uint64_t addr);
task_stat get_task_stat(uint64_t pid);
void current_task_set_priority(uint64_t priority);

// semaphore
int64_t create_semaphore(uint64_t init);
int64_t delete_semaphore(int64_t id);
bool signal_semaphore(semaphore* s, size_t count = 1);
semaphore* get_semaphore(int64_t id);
void current_task_wait_semaphore(int64_t id);
void current_task_signal_semaphore(int64_t id);
void current_task_signal_semaphore_after_microseconds(int64_t id, uint64_t microseconds);

// fd
void current_task_open(string_view path, fd_mode mode);
void current_task_create(string_view path, fd_type type);
void current_task_get_children(int64_t fd, char* buffer, size_t size);
void current_task_read(int64_t fd, uint8_t* buffer, size_t size);
void current_task_write(int64_t fd, uint8_t* buffer, size_t size);
void current_task_seek(int64_t fd, int64_t offset);
void current_task_stat(int64_t fd, fd_stat* stat);
void current_task_close(int64_t fd);
}

#endif

