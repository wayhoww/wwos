#include "syscall.h"
#include "filesystem.h"
#include "logging.h"
#include "process.h"
#include "wwos/assert.h"
#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"

namespace wwos::kernel {
    void receive_syscall(syscall_id id, uint64_t arg) {
        printf("syscall received {}\n", static_cast<uint64_t>(id));

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
            case syscall_id::SLEEP:
                schedule();
                break;
            case syscall_id::GET_PID:
            {
                auto current_task = get_current_task();
                current_task.pcb.set_return_value(current_task.pid);
                break;
            }
            case syscall_id::OPEN_FILE_OR_DIRECTORY:
            {
                const char* path = reinterpret_cast<const char*>(arg);
                auto fid = get_inode(string_view(path));
                get_current_task().pcb.set_return_value(fid);
                break;
            }
            case syscall_id::CREATE_DIRECTORY:
            {
                int64_t parent = *reinterpret_cast<int64_t*>(arg + 0);
                const char* name = *reinterpret_cast<const char**>(arg + 8);
                auto fid = create_subdirectory(parent, string_view(name));
                get_current_task().pcb.set_return_value(fid);
                break;
            }
            case syscall_id::CREATE_FILE:
            {
                int64_t parent = *reinterpret_cast<int64_t*>(arg + 0);
                const char* name = *reinterpret_cast<const char**>(arg + 8);
                auto fid = create_file(parent, string_view(name));
                get_current_task().pcb.set_return_value(fid);
                break;
            }
            case syscall_id::LIST_DIRECTORY_CHILDREN:
            {
                int64_t parent = *reinterpret_cast<int64_t*>(arg + 0);
                uint8_t* buffer = *reinterpret_cast<uint8_t**>(arg + 8);
                uint64_t size = *reinterpret_cast<uint64_t*>(arg + 16);

                if(parent < 0) {
                    get_current_task().pcb.set_return_value(-1);
                    break;
                }

                if(is_file(parent)) {
                    get_current_task().pcb.set_return_value(-1);
                    break;
                }

                auto children = get_children(parent);
                auto ret = get_flattened_children(parent, buffer, size);
                get_current_task().pcb.set_return_value(ret);
                break;
            }

            default:
                wwassert(false, "Unknown syscall id");
                break;
        }
    }
}