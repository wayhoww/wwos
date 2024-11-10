

#include "wwos/algorithm.h"
#include "wwos/assert.h"
#include "wwos/stdint.h"
#include "wwos/syscall.h"

#include "../drivers/gic2.h"
#include "../syscall.h"
#include "../process.h"
#include "interrupt.h"

namespace wwos::kernel { [[noreturn]] void internal_wwos_aarch64_handle_exception(uint64_t arg0, uint64_t arg1, uint64_t p_sp, uint64_t source); }

extern "C" [[noreturn]] void wwos_aarch64_handle_exception(wwos::uint64_t arg0, wwos::uint64_t arg1, wwos::uint64_t p_sp, wwos::uint64_t source) {
    wwos::kernel::internal_wwos_aarch64_handle_exception(arg0, arg1, p_sp, source);
}

namespace wwos::kernel {

uint64_t get_ec_bits() {
    uint64_t esr_el1;
    asm volatile(R"(
        MRS %0, esr_el1;
    )": "=r"(esr_el1) :);

    return (esr_el1 >> 26) & 0b111111;
}

uint64_t get_far_el1() {
    uint64_t far_el1;
    asm volatile(R"(
        MRS %0, far_el1;
    )": "=r"(far_el1) : :);

    return far_el1;
}

uint64_t get_spsr_el1() {
    uint64_t spsr_el1;
    asm volatile(R"(
        MRS x0, spsr_el1;
    )": "=r"(spsr_el1) : : "x0");

    return spsr_el1;
}

void setup_interrupt() {
    asm volatile(R"(
        ADR x0, aarch64_exception_vector_table;
        MSR VBAR_EL1, x0;
    )": : : "x0");

    return;
}

void save_process_info(uint64_t p_sp) {
    auto& task = get_current_task();
    uint64_t* p = reinterpret_cast<uint64_t*>(p_sp);

    for(size_t i = 0; i < 31; i++) {
        task.pcb.state.registers[i] = p[i];
    }

    task.pcb.state.spsr = p[31];
    task.pcb.pc = p[32];
    task.pcb.usp = p[33];

    task.pcb.has_return_value = false;
    task.pcb.return_value = 0xdeadbeef;
}

[[noreturn]] void eret_to_unprivileged(uint64_t addr, uint64_t sp_user, uint64_t sp_kernel, process_state state, bool withret, uint64_t ret){
    asm volatile("MSR SPSR_EL1, %0" : : "r"(state.spsr));
    asm volatile("MSR ELR_EL1, %0" : : "r"(addr));
    asm volatile("MSR SP_EL0, %0" : : "r"(sp_user));

    
    if(withret) {
        asm volatile(R"(
            MOV SP, %1

            MOV x0,  %2
            MOV x1,  %0
            LDR x2,  [x1, #16]
            LDR x3,  [x1, #24]
            LDR x4,  [x1, #32]
            LDR x5,  [x1, #40]
            LDR x6,  [x1, #48]
            LDR x7,  [x1, #56]
            LDR x8,  [x1, #64]
            LDR x9,  [x1, #72]
            LDR x10, [x1, #80]
            LDR x11, [x1, #88]
            LDR x12, [x1, #96]
            LDR x13, [x1, #104]
            LDR x14, [x1, #112]
            LDR x15, [x1, #120]
            LDR x16, [x1, #128]
            LDR x17, [x1, #136]
            LDR x18, [x1, #144]
            LDR x19, [x1, #152]
            LDR x20, [x1, #160]
            LDR x21, [x1, #168]
            LDR x22, [x1, #176]
            LDR x23, [x1, #184]
            LDR x24, [x1, #192]
            LDR x25, [x1, #200]
            LDR x26, [x1, #208]
            LDR x27, [x1, #216]
            LDR x28, [x1, #224]
            LDR x29, [x1, #232]
            LDR x30, [x1, #240]
            LDR x1,  [x1, #8]
            ERET
        )" : : "r"(&state.registers), "r"(sp_kernel), "r"(ret): "x0", "x1");

    } else {
        asm volatile(R"(
            MOV SP, %1

            MOV x0, %0
            LDR x1, [x0, #8]
            LDR x2, [x0, #16]
            LDR x3, [x0, #24]
            LDR x4, [x0, #32]
            LDR x5, [x0, #40]
            LDR x6, [x0, #48]
            LDR x7, [x0, #56]
            LDR x8, [x0, #64]
            LDR x9, [x0, #72]
            LDR x10, [x0, #80]
            LDR x11, [x0, #88]
            LDR x12, [x0, #96]
            LDR x13, [x0, #104]
            LDR x14, [x0, #112]
            LDR x15, [x0, #120]
            LDR x16, [x0, #128]
            LDR x17, [x0, #136]
            LDR x18, [x0, #144]
            LDR x19, [x0, #152]
            LDR x20, [x0, #160]
            LDR x21, [x0, #168]
            LDR x22, [x0, #176]
            LDR x23, [x0, #184]
            LDR x24, [x0, #192]
            LDR x25, [x0, #200]
            LDR x26, [x0, #208]
            LDR x27, [x0, #216]
            LDR x28, [x0, #224]
            LDR x29, [x0, #232]
            LDR x30, [x0, #240]
            LDR x0, [x0, #0]
            ERET
        )" : : "r"(&state.registers), "r"(sp_kernel): "x0");
    }
    __builtin_unreachable();
}

gic02_driver* g_interrupt_controller;

void initialize_timer() {
    constexpr size_t GICD_BASE_VA = WWOS_GICD_BASE + KA_BEGIN;
    constexpr size_t GICC_BASE_VA = WWOS_GICC_BASE + KA_BEGIN;
    constexpr size_t TIMER_IRQ = 30;
    constexpr size_t ICFGR_EDGE = 2;

    g_interrupt_controller = new gic02_driver(GICD_BASE_VA, GICC_BASE_VA);
    g_interrupt_controller->initialize();
    g_interrupt_controller->set_config(TIMER_IRQ, ICFGR_EDGE);
    g_interrupt_controller->set_priority(TIMER_IRQ, 0);
    g_interrupt_controller->clear(TIMER_IRQ);
    g_interrupt_controller->enable(TIMER_IRQ);

    return;
}

void set_timeout_interrupt(uint64_t microsecond) {
    size_t freq;
    asm volatile("MRS      %0, CNTFRQ_EL0" : "=r"(freq));
    size_t val = freq * microsecond / 1000000;
    val = max<size_t>(val, 1000ull);
    asm volatile("MSR      CNTP_TVAL_EL0, %0" : : "r"(val));

    asm volatile(
        R"(
            MOV      x0, 1
            MSR      CNTP_CTL_EL0, x0
        )":::"x0"
    );
}

void enable_irq() {
    asm volatile( "MSR DAIFCLR, #0b0010" ); 
}

void disable_irq() {
    asm volatile( "MSR DAIFSET, #0b0010" );
}


constexpr size_t TIMER_IRQ = 30;

[[noreturn]] void internal_wwos_aarch64_handle_exception(uint64_t arg0, uint64_t arg1, uint64_t p_sp, uint64_t source) {
    save_process_info(p_sp);

    auto ec_bits = get_ec_bits();

    int64_t interrupt_id = 1023;
    
    // {
    //     auto current_task = get_current_task();
    //     wwfmtlog("Exception. ELR={:x}, SP={:x} ARG0={:x} ARG1={:x} SOURCE={:x}, EC={:x}", current_task.pcb.pc, current_task.pcb.usp, arg0, arg1, source, ec_bits);
    // }

    if(source % 4 == 1) {
        if(g_interrupt_controller && ((interrupt_id = g_interrupt_controller->get_interrupt_id()) != 1023)) {
            g_interrupt_controller->finish_interrupt(interrupt_id);
            if(interrupt_id == TIMER_IRQ) {
                on_timeout();
            } else {
                wwassert(false, "Unknown interrupt");
            }
        }
    } else {
        if(ec_bits == 0b000000) {
            wwassert(false, "Unknown reason: not supported yet");        
        } else if(ec_bits <= 0b010000) {
            wwassert(false, "Trap: not supported yet");
        } else if(ec_bits == 0b010001 || ec_bits == 0b010101) {
            receive_syscall(static_cast<syscall_id>(arg0), arg1);
        } else if(ec_bits == 0b100100 || ec_bits == 0b100101) {
            auto far = get_far_el1();
            on_data_abort(far);
        } else {
            wwassert(false, "Other reason: not supported yet");
        }
    }

    auto& current_task = get_current_task();
    current_task.pcb.tt.activate();
#ifdef WWOS_LOG_ERET
    wwfmtlog("ret?={}, ret_value={}", current_task.pcb.has_return_value, current_task.pcb.return_value);
    wwfmtlog("eret to unprivileged. pid={}, pc={:x} usp={:x}", current_task.pid, current_task.pcb.pc, current_task.pcb.usp);
#endif
    eret_to_unprivileged(current_task.pcb.pc, current_task.pcb.usp, current_task.pcb.ksp, current_task.pcb.state, current_task.pcb.has_return_value, current_task.pcb.return_value);
}



}