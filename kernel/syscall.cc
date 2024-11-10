#include "syscall.h"
#include "filesystem.h"
#include "logging.h"
#include "process.h"
#include "wwos/assert.h"
#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"

namespace wwos::kernel {
    void receive_syscall(syscall_id id, uint64_t arg) {
        switch (id) {
        case syscall_id::PUTCHAR:
            kputchar(arg);
            break;
        case syscall_id::GETCHAR:
            get_current_task().pcb.set_return_value(kgetchar());
            break;
        case syscall_id::ALLOC:
            kallocate_page(arg);
            break;
        case syscall_id::FORK:
            fork_current_task();
            break;
        case syscall_id::EXEC:
            replace_current_task(string_view(reinterpret_cast<const char*>(arg)));
            break;
        case syscall_id::SEMAPHORE_CREATE:
            get_current_task().pcb.set_return_value(create_semaphore(arg));
            break;
        case syscall_id::SEMAPHORE_DESTROY:
            get_current_task().pcb.set_return_value(delete_semaphore(arg));
            break;
        case syscall_id::SEMAPHORE_SIGNAL:
            current_task_signal_semaphore(arg);
            break;
        case syscall_id::SEMAPHORE_SIGNAL_AFTER_MICROSECONDS: 
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            current_task_signal_semaphore_after_microseconds(params[0], params[1]);
            break;    
        }
        case syscall_id::SEMAPHORE_WAIT:
            current_task_wait_semaphore(arg);
            break;
        case syscall_id::GET_PID:
        {
            auto& current_task = get_current_task();
            wwfmtlog("pid = {}\n", current_task.pid);
            current_task.pcb.set_return_value(current_task.pid);
            break;
        }
        case syscall_id::FD_OPEN:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            if(params[0] >= KA_BEGIN) {
                get_current_task().pcb.set_return_value(-1);
                break;
            }
            current_task_open(string_view(reinterpret_cast<const char*>(params[0])), static_cast<fd_mode>(params[1]));
            break;
        }
        case syscall_id::FD_CLOSE:
            current_task_close(arg);
            break;
        case syscall_id::FD_CREATE:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            if(params[0] >= KA_BEGIN) {
                get_current_task().pcb.set_return_value(-1);
                break;
            }
            current_task_create(string_view(reinterpret_cast<const char*>(params[0])), static_cast<fd_type>(params[1]));
            break;
        }
        case syscall_id::FD_CHILDREN:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            if(params[1] >= KA_BEGIN) {
                get_current_task().pcb.set_return_value(-1);
                break;
            }
            current_task_get_children(params[0], reinterpret_cast<char*>(params[1]), params[2]);
            break;
        }
        case syscall_id::FD_READ:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            if(params[1] >= KA_BEGIN) {
                get_current_task().pcb.set_return_value(-1);
                break;
            }
            current_task_read(params[0], reinterpret_cast<uint8_t*>(params[1]), params[2]);
            break;
        }
        case syscall_id::FD_WRITE:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            if(params[1] >= KA_BEGIN) {
                get_current_task().pcb.set_return_value(-1);
                break;
            }
            current_task_write(params[0], reinterpret_cast<uint8_t*>(params[1]), params[2]);
            break;
        }
        case syscall_id::FD_SEEK:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            current_task_seek(params[0], params[1]);
            break;
        }
        case syscall_id::FD_STAT:
        {
            uint64_t* params = reinterpret_cast<uint64_t*>(arg);
            if(params[1] >= KA_BEGIN) {
                get_current_task().pcb.set_return_value(-1);
                break;
            }
            current_task_stat(params[0], reinterpret_cast<fd_stat*>(params[1]));
            break;
        }
        case wwos::syscall_id::TASK_STAT:
            get_current_task().pcb.set_return_value((uint64_t)get_task_stat(arg));
            break;
        case syscall_id::EXIT:
            current_task_exit();
            break;
        default:
            wwassert(false, "Unknown syscall id");
            break;
        }
    }
}