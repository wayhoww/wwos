#include "aarch64/interrupt.h"
#include "aarch64/memory.h"
#include "wwos/alloc.h"
#include "wwos/assert.h"
#include "wwos/defs.h"
#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/vector.h"

#include "process.h"
#include "global.h"
#include "filesystem.h"
#include "arch.h"

namespace wwos::kernel {
    uint64_t current_pid = 0;


    vector<task_info>* p_task_list;
    size_t schedule_index = 0;
    size_t pid_counter = 0;

    void load_program(translation_table_user& ttu, string_view binary) {
        // print binary size

        for(size_t i = 0; i < binary.size(); i += translation_table_user::PAGE_SIZE) {
            auto pa = pallocator->alloc();
            ttkernel->set_page(pa, pa);
            ttkernel->activate();

            auto va = pa + KA_BEGIN;
            uint8_t* p = reinterpret_cast<uint8_t*>(va);

            for(size_t j = 0; j < translation_table_user::PAGE_SIZE; j++) {
                if(i + j >= binary.size()) {
                    break;
                }
                *p++ = binary[i + j];
            }
            
            ttu.set_page(USERSPACE_TEXT + i, pa);
        }
        
        {
            // load stack
            auto pa = pallocator->alloc();
            ttkernel->set_page(pa, pa);
            ttkernel->activate();

            ttu.set_page(USERSPACE_STACK_TOP - translation_table_user::PAGE_SIZE, pa);
        }
    }


    task_info* get_task(uint64_t pid) {
        for(auto& task : *p_task_list) {
            if(task.pid == pid) {
                return &task;
            }
        }

        return nullptr;
    }

    uint64_t fork_task(uint64_t parent_pid) {
        auto parent = get_task(parent_pid);
        wwassert(parent != nullptr, "Invalid parent pid");


        // do not copy kernel stack.
        auto kernel_stack = pallocator->alloc();
        ttkernel->set_page(kernel_stack, kernel_stack);
        ttkernel->activate();


        task_info task = {
            .pid = pid_counter++,
            .pcb = {
                .pc = parent->pcb.pc,
                .ksp = KA_BEGIN + kernel_stack + translation_table_kernel::PAGE_SIZE,
                .usp = parent->pcb.usp,
                .has_return_value = true,
                .return_value = 0,
                .state = parent->pcb.state,
                .tt = new translation_table_user(),  
            }
        };

        parent->pcb.set_return_value(task.pid);

        auto pages = parent->pcb.tt->get_all_pages();
        for(auto& [va, pa] : pages) {
            auto new_pa = pallocator->alloc();
            ttkernel->set_page(new_pa, new_pa);
            ttkernel->activate();
            auto new_va_kernel = new_pa + KA_BEGIN;
            memcpy(reinterpret_cast<void*>(new_va_kernel), reinterpret_cast<void*>(pa + KA_BEGIN), translation_table_kernel::PAGE_SIZE);
            task.pcb.tt->set_page(va, new_pa);
        }
        
        p_task_list->push_back(task);
        return task.pid;
    }

    void initialize_process_subsystem() {
        p_task_list = new vector<task_info>();
        schedule_index = 0;
        pid_counter = 0;
    }

    void create_process(string_view path, uint64_t pid) {
        // 1. load program
        // 2. add to process list
        auto kernel_stack = pallocator->alloc();
        ttkernel->set_page(kernel_stack, kernel_stack);
        ttkernel->activate();

        pid = pid == -1 ? pid_counter++ : pid;

        task_info task = {
            .pid = pid,
            .pcb = {
                .pc = USERSPACE_TEXT,
                .ksp = KA_BEGIN + kernel_stack + translation_table_kernel::PAGE_SIZE,
                .usp = USERSPACE_STACK_TOP,
                .has_return_value = false,
                .return_value = 0,
                .state = process_state(),
                .tt = new translation_table_user(),  
            }
        };

        auto inode = get_inode(path);
        auto inode_size = get_inode_size(inode);
        vector<uint8_t> binary(inode_size);

        read_inode(binary.data(), inode, 0, inode_size);
        load_program(*task.pcb.tt, binary);

        p_task_list->push_back(task);
    }

    void replace_task(uint64_t pid, string_view path) {
        auto task = get_task(pid);
        wwassert(task != nullptr, "Invalid pid");
        // todo: destroy old task
        p_task_list->erase(task);
        create_process(path, pid);
    }

    [[noreturn]] void schedule() {
        wwassert(p_task_list->size() > 0, "empty task list");

        schedule_index++;

        if(schedule_index >= p_task_list->size()) {
            schedule_index = 0;
        }

        auto& task = (*p_task_list)[schedule_index];
        current_pid = task.pid;

        set_timeout_interrupt(100000);
        task.pcb.tt->activate();
        eret_to_unprivileged(task.pcb.pc, task.pcb.usp, task.pcb.ksp, task.pcb.state, task.pcb.has_return_value, task.pcb.return_value);

        __builtin_unreachable();
    }

    task_info& get_current_task() {
        return (*p_task_list)[schedule_index];
    }

    [[noreturn]] void on_timeout() {
        schedule();
    }

    void kallocate_page(uint64_t va) {
        auto& current_task = get_current_task();
        
        if(va % translation_table_user::PAGE_SIZE != 0) {
            current_task.pcb.set_return_value(false);
            return;
        }

        if(va < USERSPACE_HEAP || va >= USERSPACE_HEAP_END) {
            current_task.pcb.set_return_value(false);
            return;
        }

        auto all_pages = current_task.pcb.tt->get_all_pages();
        for(auto& [v, p] : all_pages) {
            if(v == va) {
                current_task.pcb.set_return_value(false);
                return;
            }
        }

        auto pa = pallocator->alloc();
        ttkernel->set_page(pa, pa);
        ttkernel->activate();
        current_task.pcb.tt->set_page(va, pa);
        current_task.pcb.tt->activate();
        current_task.pcb.set_return_value(true);
        return;
    }
    

}