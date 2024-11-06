#ifndef _WWOS_SYSCALL_H
#define _WWOS_SYSCALL_H

#include "wwos/algorithm.h"
#include "wwos/assert.h"
#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/string_view.h"
#include "wwos/vector.h"
#include "wwos/string.h"

namespace wwos {

    enum class syscall_id: uint64_t {
        // io
        PUTCHAR,
        GETCHAR,

        // memory
        ALLOC,

        // process
        FORK,
        EXEC,
        SLEEP,
        GET_PID,

        // file system
        OPEN_FILE_OR_DIRECTORY,    
        CREATE_DIRECTORY,
        LIST_DIRECTORY_CHILDREN,
        CREATE_FILE,
        READ_FILE,
        WRITE_FILE,
        FILE_GET_SIZE,
        FILE_SET_SIZE,
    };

    inline uint64_t syscall(syscall_id id, uint64_t arg) {
        uint64_t ret;
        asm volatile(R"(
            MOV x10, %1;
            MOV x11, %2;
            SVC #0;
            MOV %0, x0;
        )": "=r"(ret) : "r"(id), "r"(arg) : "x10", "x11");

        return ret;
    }

    inline void sleep(uint64_t ms) {
        syscall(syscall_id::SLEEP, ms);
    }

    inline int64_t fork() {
        return syscall(syscall_id::FORK, 0);
    }

    inline bool allocate_page(uint64_t va) {
        return syscall(syscall_id::ALLOC, va);
    }

    [[noreturn]] inline void exec(string_view path) {
        syscall(syscall_id::EXEC, reinterpret_cast<uint64_t>(path.data()));
        __builtin_unreachable();
    }

    inline uint64_t open(string_view path) {
        return syscall(syscall_id::OPEN_FILE_OR_DIRECTORY, reinterpret_cast<uint64_t>(path.data()));
    }

    inline vector<pair<string, size_t>> get_directory_children(uint64_t id) {
        vector<uint8_t> buffer(4096);
        uint64_t params[] = {id, reinterpret_cast<uint64_t>(buffer.data()), buffer.size()};
        int64_t ret = syscall(syscall_id::LIST_DIRECTORY_CHILDREN, reinterpret_cast<uint64_t>(params));

        if(ret < 0) {
            return {};
        }

        if(ret > 0) {
            buffer = vector<uint8_t>(ret);
            params[1] = reinterpret_cast<uint64_t>(buffer.data());
            params[2] = ret;
            auto ret = syscall(syscall_id::LIST_DIRECTORY_CHILDREN, reinterpret_cast<uint64_t>(params));
            wwassert(ret == 0, "Failed to get directory children");
        }

        vector<pair<string, size_t>> out;
        size_t i = 0;
        while(i < buffer.size()) {
            string name(reinterpret_cast<char*>(buffer.data() + i));
            if(name.size() == 0) {
                break;
            }
            i += align_up<size_t>(name.size() + 1, 8);
            size_t id = *reinterpret_cast<size_t*>(buffer.data() + i);
            i += 8;
            out.push_back({name, id});
        }

        return out;
    }

    inline int64_t make_directory(int64_t parent, string_view name) {
        uint64_t params[] = {static_cast<uint64_t>(parent), reinterpret_cast<uint64_t>(name.data())};
        return syscall(syscall_id::CREATE_DIRECTORY, reinterpret_cast<uint64_t>(params));
    }
}

#endif