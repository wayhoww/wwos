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

    enum class fd_type: uint64_t {
        FILE,
        DIRECTORY,
        FIFO
    };

    enum class fd_mode: uint64_t {
        READONLY,
        WRITEONLY,
    };

    struct fd_stat {
        uint64_t size;
        fd_type type;
    };

    enum class syscall_id: uint64_t {
        // io
        PUTCHAR,
        GETCHAR,

        // memory
        ALLOC,

        // process
        FORK,
        EXEC,
        GET_PID,

        // semaphore
        SEMAPHORE_CREATE, 
        SEMAPHORE_SIGNAL,
        SEMAPHORE_SIGNAL_AFTER_MICROSECONDS,
        SEMAPHORE_WAIT,
        SEMAPHORE_DESTROY,

        // file system
        FD_OPEN,        // path, mode           -> fd / <0
        FD_CLOSE,       // fd                   -> 0 / <0
        FD_CREATE,      // path, type           -> 0 / <0
        FD_CHILDREN,    // fd, buffer, size     -> extra size required / 0 / <0
        FD_READ,        // fd, buffer, size     -> write size / <0
        FD_WRITE,       // fd, buffer, size     -> read size / <0
        FD_SEEK,        // fd, offset           -> 0 / <0
        FD_STAT,        // fd, &stat            -> 0 / <0
    };

    inline uint64_t syscall(syscall_id id, uint64_t arg) {
        uint64_t ret;
        asm volatile(R"(
            MOV x10, %1;
            MOV x11, %2;
            SVC #0;
            MOV %0, x0;
        )": "=r"(ret) : "r"(id), "r"(arg) : "x10", "x11", "x0");

        return ret;
    }

    inline uint64_t semaphore_create(int64_t count) {
        return syscall(syscall_id::SEMAPHORE_CREATE, count);
    }

    inline int64_t semaphore_signal(int64_t id) {
        return syscall(syscall_id::SEMAPHORE_SIGNAL, id);
    }

    inline int64_t semaphore_signal_after_microseconds(int64_t id, uint64_t microseconds) {
        uint64_t params[] = {(uint64_t)id, microseconds};
        return syscall(syscall_id::SEMAPHORE_SIGNAL_AFTER_MICROSECONDS, reinterpret_cast<uint64_t>(params));
    }

    inline int64_t semaphore_wait(int64_t id) {
        return syscall(syscall_id::SEMAPHORE_WAIT, id);
    }

    inline int64_t semaphore_destroy(int64_t id) {
        return syscall(syscall_id::SEMAPHORE_DESTROY, id);
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

    inline int64_t open(string_view path, fd_mode mode) {
        uint64_t params[] = {reinterpret_cast<uint64_t>(path.data()), static_cast<uint64_t>(mode)};
        return syscall(syscall_id::FD_OPEN, reinterpret_cast<uint64_t>(params));
    }

    inline int64_t create(string_view path, fd_type type) {
        uint64_t params[] = {reinterpret_cast<uint64_t>(path.data()), static_cast<uint64_t>(type)};
        return syscall(syscall_id::FD_CREATE, reinterpret_cast<uint64_t>(params));
    }

    int64_t get_children(int64_t fd, vector<string>& buffer);

    inline size_t read(int64_t fd, uint8_t* buffer, size_t size) {
        uint64_t params[] = {static_cast<uint64_t>(fd), reinterpret_cast<uint64_t>(buffer), size};
        return syscall(syscall_id::FD_READ, reinterpret_cast<uint64_t>(params));
    }

    inline size_t write(int64_t fd, uint8_t* buffer, size_t size) {
        uint64_t params[] = {static_cast<uint64_t>(fd), reinterpret_cast<uint64_t>(buffer), size};
        return syscall(syscall_id::FD_WRITE, reinterpret_cast<uint64_t>(params));
    }

    inline int64_t seek(int64_t fd, size_t offset) {
        uint64_t params[] = {static_cast<uint64_t>(fd), offset};
        return syscall(syscall_id::FD_SEEK, reinterpret_cast<uint64_t>(params));
    }

    inline int64_t stat(int64_t fd, fd_stat* s) {
        uint64_t params[] = {static_cast<uint64_t>(fd), reinterpret_cast<uint64_t>(s)};
        return syscall(syscall_id::FD_STAT, reinterpret_cast<uint64_t>(params));
    }

    inline int64_t close(int64_t fd) {
        return syscall(syscall_id::FD_CLOSE, fd);
    }

    inline int64_t get_pid() {
        return syscall(syscall_id::GET_PID, 0);
    }
}

#endif