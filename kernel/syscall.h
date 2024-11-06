#ifndef _WWOS_KERNEL_SYS_CALL_H
#define _WWOS_KERNEL_SYS_CALL_H


#include "wwos/syscall.h"
namespace wwos::kernel {
    void receive_syscall(syscall_id id, uint64_t arg);
}


#endif