#include "wwos/algorithm.h"
#include "wwos/alloc.h"
#include "wwos/assert.h"
#include "wwos/avl.h"
#include "wwos/defs.h"
#include "wwos/format.h"
#include "wwos/map.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"
#include "wwos/vector.h"

#include "scheduler.h"
#include "process.h"
#include "global.h"
#include "filesystem.h"
#include "arch.h"

namespace wwos::kernel {
    
    struct clock_info {
        int64_t semaphore_id;
        uint64_t expiration_time;

        bool operator<(const clock_info& other) const {
            return expiration_time < other.expiration_time;
        }
    };

    scheduler* p_scheduler = nullptr;
    map<uint64_t, semaphore*>* p_semaphores;
    avl_tree<clock_info>* p_clock_tree;

    int64_t pid_counter = 0;
    int64_t semaphore_counter = 0;

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

    void init_fifo_for_process(uint64_t pid) {
        wwfmtlog("initing fifo for pid {}", pid);

        auto proc = open_shared_file_node(0, "/proc", fd_mode::READONLY);
        if(proc == nullptr) {
            create_shared_file_node("/proc", fd_type::DIRECTORY);
        } else {
            close_shared_file_node(0, proc);
        }

        create_shared_file_node(format("/proc/{}", pid), fd_type::DIRECTORY);
        create_shared_file_node(format("/proc/{}/fifo", pid), fd_type::DIRECTORY);
        create_shared_file_node(format("/proc/{}/fifo/stdin", pid), fd_type::FIFO);
        create_shared_file_node(format("/proc/{}/fifo/stdout", pid), fd_type::FIFO);
    }

    void fork_current_task() {
        auto parent = p_scheduler->get_executing_task();
        wwassert(parent != nullptr, "Invalid parent pid");

        wwfmtlog("forking. parent={}\n", parent->pid);

        // do not copy kernel stack.
        auto kernel_stack = pallocator->alloc(KERNEL_STACK_SIZE / translation_table_kernel::PAGE_SIZE);
        static_assert(KERNEL_STACK_SIZE % translation_table_kernel::PAGE_SIZE == 0);
        for(size_t i = 0; i < KERNEL_STACK_SIZE; i += translation_table_kernel::PAGE_SIZE) {
            ttkernel->set_page(kernel_stack + i, kernel_stack + i);
        }
        ttkernel->set_page(kernel_stack, kernel_stack);
        ttkernel->activate();


        task_info* task = new task_info {
            .pid = uint64_t(pid_counter++),
            .pcb = {
                .pc = parent->pcb.pc,
                .ksp = KA_BEGIN + kernel_stack + KERNEL_STACK_SIZE,
                .usp = parent->pcb.usp,
                .has_return_value = true,
                .return_value = 0,
                .state = parent->pcb.state,
                .tt = new translation_table_user(),  
            },
            .fd_counter = parent->fd_counter,
        };

        wwfmtlog("forked. parent={}, child={}", parent->pid, task->pid);
        // copy fds, including stdin, stdout
        auto fd_copy = parent->fds.items();
        for(size_t i = 0; i < fd_copy.size(); i++) {
            auto fd_info = fd_copy[i].second;

            for(auto& pid : fd_info.node->readers) {
                if(pid == parent->pid) {
                    fd_info.node->readers.push_back(task->pid);
                }
            }
            for(auto& pid : fd_info.node->writers) {
                if(pid == parent->pid) {
                    fd_info.node->writers.push_back(task->pid);
                }
            }
        }
        task->fds = parent->fds;

        init_fifo_for_process(task->pid);

        parent->pcb.set_return_value(task->pid);

        auto pages = parent->pcb.tt->get_all_pages();
        for(auto& [va, pa] : pages) {
            auto new_pa = pallocator->alloc();
            ttkernel->set_page(new_pa, new_pa);
            ttkernel->activate();
            auto new_va_kernel = new_pa + KA_BEGIN;
            memcpy(reinterpret_cast<void*>(new_va_kernel), reinterpret_cast<void*>(pa + KA_BEGIN), translation_table_kernel::PAGE_SIZE);
            task->pcb.tt->set_page(va, new_pa);
        }        
        p_scheduler->add_task(task);
    }

    void initialize_process_subsystem() {
        p_scheduler = new scheduler();
        p_semaphores = new map<uint64_t, semaphore*>();
        p_clock_tree = new avl_tree<clock_info>();
        pid_counter = 0;
        semaphore_counter = 0;
    }

    int64_t create_semaphore(uint64_t init) {
        int64_t id = semaphore_counter++;
        semaphore* s = new semaphore(init);
        p_semaphores->insert(id, s);
        return id;
    }

    int64_t delete_semaphore(int64_t id) {
        if(!p_semaphores->contains(id)) {
            return -1;
        }
        auto s = p_semaphores->get(id);
        if(s->waiting_tasks.size() > 0) {
            return -2;
        }
        p_semaphores->remove(id);
        delete s;
        return 0;
    }

    void current_task_wait_semaphore(int64_t id) {
        auto task = p_scheduler->get_executing_task();
        wwassert(task != nullptr, "no executing task");

        if(!p_semaphores->contains(id)) {
            task->pcb.set_return_value(-1);
            return;
        }

        auto s = p_semaphores->get(id);

        if(s->count > 0) {
            s->count--;
        } else {
            p_scheduler->remove_task(task);
            s->waiting_tasks.push_back(task);
        }
    }

    bool signal_semaphore(semaphore* s, size_t count) {
        if(s->count == ~0ull and s->waiting_tasks.size() == 0) {
            return false;
        }

        while(count > 0 and s->waiting_tasks.size() > 0) {
            auto task = s->waiting_tasks.back();
            s->waiting_tasks.pop_back();
            task->pcb.set_return_value(0);
            p_scheduler->add_task(task);
            count--;
        }

        s->count += count;

        return true;
    }

    semaphore* get_semaphore(int64_t id) {
        // for(auto i: p_semaphores->items()) {
        //     if(i.first == id) {
        //         return i.second;        // TODO: change it..
        //     }
        // }

        if(!p_semaphores->contains(id)) {
            wwlog("semaphore not found\n");
            return nullptr;
        }
        return p_semaphores->get(id);
    }

    void current_task_signal_semaphore(int64_t id) {
        auto task = p_scheduler->get_executing_task();
        wwassert(task != nullptr, "no executing task");

        if(!p_semaphores->contains(id)) {
            task->pcb.set_return_value(-1);
            return;
        }

        auto s = p_semaphores->get(id);
        if(signal_semaphore(s)) {
            task->pcb.set_return_value(0);
        } else {
            task->pcb.set_return_value(-2);
        }
    }

    void current_task_signal_semaphore_after_microseconds(int64_t id, uint64_t microseconds) {
        auto task = p_scheduler->get_executing_task();
        wwassert(task != nullptr, "no executing task");

        if(!p_semaphores->contains(id)) {
            task->pcb.set_return_value(-1);
            return;
        }

        task->pcb.set_return_value(0);
        p_clock_tree->insert({id, get_cpu_time() + microseconds});
    }

    void signal_semaphore_by_clocks() {
        auto current_time = get_cpu_time();
        while(!p_clock_tree->empty() && p_clock_tree->smallest()->data.expiration_time <= current_time) {
            auto smallest = p_clock_tree->smallest();
            auto id = smallest->data.semaphore_id;
            p_clock_tree->remove(smallest);
            if(!p_semaphores->contains(id)) {
                continue;
            }
            auto s = p_semaphores->get(id);
            signal_semaphore(s); // discarded result
        }
    }

    void create_process(string_view path, task_info* replacing) {
        // 1. load program
        // 2. add to process list
        auto kernel_stack = pallocator->alloc(KERNEL_STACK_SIZE / translation_table_kernel::PAGE_SIZE);
        for(size_t i = 0; i < KERNEL_STACK_SIZE; i += translation_table_kernel::PAGE_SIZE) {
            ttkernel->set_page(kernel_stack + i, kernel_stack + i);
        }
        ttkernel->activate();

        uint64_t pid = replacing == nullptr ? pid_counter++ : replacing->pid;

        task_info* task = new task_info {
            .pid = pid,
            .pcb = {
                .pc = USERSPACE_TEXT,
                .ksp = KA_BEGIN + kernel_stack + KERNEL_STACK_SIZE,
                .usp = USERSPACE_STACK_TOP,
                .has_return_value = false,
                .return_value = 0,
                .state = process_state(),
                .tt = new translation_table_user(),  
            }
        };

        auto sfn = open_shared_file_node(0, path, fd_mode::READONLY);
        wwassert(sfn, "failed to open file");
        
        auto size = get_shared_node_size(sfn);
        vector<uint8_t> binary(size);
        
        wwassert(read_shared_node(binary.data(), sfn, 0, size) == size, "failed to read file");
        close_shared_file_node(0, sfn);

        load_program(*task->pcb.tt, binary);

        if(replacing == nullptr) {
            init_fifo_for_process(pid);
        }

        if(replacing != nullptr) {
            p_scheduler->replace_task(replacing, task);
        } else {
            p_scheduler->add_task(task);
        }
    }

    void replace_current_task(string_view path) {
        auto task = p_scheduler->get_executing_task();
        wwassert(task != nullptr, "Invalid pid");

        create_process(path, task);

        wwassert(p_scheduler->get_executing_task() != nullptr, "no executing task");
    }

    [[noreturn]] void schedule() {
        signal_semaphore_by_clocks();

        auto task = p_scheduler->schedule();
        wwassert(task != nullptr, "no task to schedule");

        // dump ret
        wwfmtlog("ret?={}, ret_value={}", task->pcb.has_return_value, task->pcb.return_value);
        wwfmtlog("scheduled. eret to unprivileged. pid={}, pc={:x} usp={:x}", task->pid, task->pcb.pc, task->pcb.usp);
        task->pcb.tt->activate();
        set_timeout_interrupt(10000);

        eret_to_unprivileged(
            task->pcb.pc, task->pcb.usp, task->pcb.ksp, task->pcb.state, 
            task->pcb.has_return_value, task->pcb.return_value);

        __builtin_unreachable();
    }

    task_info& get_current_task() {
        auto current_task = p_scheduler->get_executing_task();
        wwassert(current_task, "no executing task");

        return *current_task;
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

    bool check_pointer_validity(uint64_t va, uint64_t size) {
        // assure kernel space are not accessed
        // TODO handle data abort (terminate corresponding task)

        if(va >= KA_BEGIN) {
            // in case of overflow
            return false;
        }
        if(va + size >= KA_BEGIN) {
            return false;
        }

        return true;
    }

    void current_task_open(string_view path, fd_mode mode) {
        auto current_task = p_scheduler->get_executing_task();
        if(!check_pointer_validity((uint64_t)path.data(), path.size())) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        wwfmtlog("trying to open {} with mode {}. by {}", path, static_cast<int>(mode), current_task->pid);
        
        auto sfn = open_shared_file_node(current_task->pid, path, mode);
        if(sfn == nullptr) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        uint64_t fd = current_task->fd_counter++;
        wwfmtlog("assigned fd {} for pid {}", fd, current_task->pid);

        current_task->fds.insert(fd, fd_info {.node = sfn, .mode = mode, .offset = 0});

        wwfmtlog("inserted fd {} for pid {}. mode={}", fd, current_task->pid, static_cast<int>(current_task->fds.get(fd).mode));
        
        current_task->pcb.set_return_value(fd);
    }

    void current_task_create(string_view path, fd_type type) {
        auto current_task = p_scheduler->get_executing_task();
        if(!check_pointer_validity((uint64_t)path.data(), path.size())) {
            current_task->pcb.set_return_value(-1);
            return;
        }  

        auto sfn = create_shared_file_node(path, type);
        if(!sfn) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        current_task->pcb.set_return_value(0);
    }

    void current_task_read(int64_t fd, uint8_t* buffer, size_t size) {
        // wwlog("request read");
        auto current_task = p_scheduler->get_executing_task();
        wwassert(current_task, "no executing task");

        if(!check_pointer_validity((uint64_t)buffer, size)) {
            current_task->pcb.set_return_value(-1);
            wwlog("invalid buffer");
            return;
        }

        if(!current_task->fds.contains(fd)) {
            current_task->pcb.set_return_value(-2);
            wwfmtlog("invalid fd {} for pid {}", fd, current_task->pid);
            return;
        }

        auto& fd_info = current_task->fds.get(fd);
        if(fd_info.mode != fd_mode::READONLY) {
            current_task->pcb.set_return_value(-3);
            wwfmtlog("invalid mode {} for pid {} fd {}", static_cast<int>(fd_info.mode), current_task->pid, fd);
            return;
        }
        
        auto read_size = read_shared_node(buffer, fd_info.node, fd_info.offset, size);
        fd_info.offset += read_size;
        
        // wwfmtlog("read_size={}", read_size);
        current_task->pcb.set_return_value(read_size);
    }

    void current_task_write(int64_t fd, uint8_t* buffer, size_t size) {
        auto current_task = p_scheduler->get_executing_task();
        if(!check_pointer_validity((uint64_t)buffer, size)) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        if(!current_task->fds.contains(fd)) {
            current_task->pcb.set_return_value(-2);
            return;
        }

        auto& fd_info = current_task->fds.get(fd);
        if(fd_info.mode != fd_mode::WRITEONLY) {
            current_task->pcb.set_return_value(-3);
            return;
        }

        auto write_size = write_shared_node(buffer, fd_info.node, fd_info.offset, size);
        fd_info.offset += write_size;
        current_task->pcb.set_return_value(write_size);
    }

    void current_task_seek(int64_t fd, int64_t offset) {
        auto current_task = p_scheduler->get_executing_task();
        if(!current_task->fds.contains(fd)) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        auto& fd_info = current_task->fds.get(fd);
        if(offset < 0 || offset >= get_shared_node_size(fd_info.node)) {
            current_task->pcb.set_return_value(-2);
            return;
        }

        fd_info.offset = offset;
        current_task->pcb.set_return_value(0);
    }

    void current_task_stat(int64_t fd, fd_stat* stat) {
        auto current_task = p_scheduler->get_executing_task();
        if(!current_task->fds.contains(fd)) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        auto& fd_info = current_task->fds.get(fd);
        stat->size = get_shared_node_size(fd_info.node);
        stat->type = fd_info.node->type;
        current_task->pcb.set_return_value(0);
    }

    void current_task_get_children(int64_t fd, char* buffer, size_t size) {
        auto current_task = p_scheduler->get_executing_task();
        if(!check_pointer_validity((uint64_t)buffer, size)) {
            current_task->pcb.set_return_value(-1);
            return;
        }

        if(!current_task->fds.contains(fd)) {
            current_task->pcb.set_return_value(-2);
            return;
        }

        auto& fd_info = current_task->fds.get(fd);
        if(fd_info.node->type != fd_type::DIRECTORY) {
            current_task->pcb.set_return_value(-3);
            return;
        }

        auto flatten_children = get_flattened_children(fd_info.node, reinterpret_cast<uint8_t*>(buffer), size);
        current_task->pcb.set_return_value(flatten_children);
    }

    void current_task_close(int64_t fd) {
        auto current_task = p_scheduler->get_executing_task();
        if(!current_task->fds.contains(fd)) {
            current_task->pcb.set_return_value(-1);
            return;
        }
        
        auto& fd_info = current_task->fds.get(fd);
        close_shared_file_node(current_task->pid, fd_info.node);

        current_task->fds.remove(fd);
        current_task->pcb.set_return_value(0);
    }

    void on_data_abort(uint64_t addr) {
        auto addr_aligned_down = align_down(addr, translation_table_user::PAGE_SIZE);
        if(addr_aligned_down >= USERSPACE_STACK_BOTTOM && addr_aligned_down <= USERSPACE_STACK_TOP) {
            auto& current_task = get_current_task();
            auto pa =  pallocator->alloc();
            ttkernel->set_page(pa, pa);
            current_task.pcb.tt->set_page(addr_aligned_down, pa);
            current_task.pcb.tt->activate();
            wwfmtlog("allocated page for data abort. addr={:x} pa={:x}", addr_aligned_down, pa);
            return;
        }
        
        wwassert(false, "data abort");
        
    }
}