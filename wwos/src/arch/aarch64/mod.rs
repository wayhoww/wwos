mod memory;
mod process;

pub use memory::*;
pub use process::*;

use core::arch::asm;

use alloc::boxed::Box;
use log::info;

use crate::{process::Process, SHARED_REGION_BEGIN};

unsafe fn read_mpidr_el1() -> u64 {
    let mut mpidr_el1: u64;
    asm!("mrs {x}, mpidr_el1", x = out(reg) mpidr_el1,);
    mpidr_el1
}

unsafe fn get_core_id_el1() -> u32 {
    let mpidr_el1 = read_mpidr_el1();
    mpidr_el1 as u32 & 0x3
}

fn halt_core() -> ! {
    unsafe {
        asm!(
            r#"
            1:  wfe;
                b       1b;
            "#
        )
    }
    panic!("Unreachable");
}

unsafe fn set_exception_vector_table_el1(addr: usize) {
    asm!("msr vbar_el1, {addr}", addr = in(reg) addr);
}

unsafe fn initialize_exception_vector_table_el1() {
    let mut addr_vt: usize;
    asm!("adr {addr_vt}, aarch64_exception_vector_table", addr_vt = out(reg) addr_vt);
    set_exception_vector_table_el1(addr_vt);
}

unsafe fn get_spsr_el2() -> usize {
    let mut spsr_el2: usize;
    asm!("mrs {x}, spsr_el2", x = out(reg) spsr_el2);
    spsr_el2
}

unsafe fn set_spsr_el2(spsr_el2: usize) {
    asm!("msr spsr_el2, {x}", x = in(reg) spsr_el2);
}

unsafe fn switch_to_el1() {
    if get_current_el() == 1 {
        return;
    }

    if get_current_el() != 2 {
        panic!("Unsupported current EL");
    }

    let _tmp: usize;
    let _tmp2: usize;

    asm!(
        "
        // Configure HCR_EL2
        // ------------------
        ORR      w0, wzr, #(1 << 3)             // FMO=1
        ORR      x0, x0,  #(1 << 4)             // IMO=1
        ORR      x0, x0,  #(1 << 31)            // RW=1          NS.EL1 is AArch64
                                                // TGE=0         Entry to NS.EL1 is possible
                                                // VM=0          Stage 2 MMU disabled
        MSR      HCR_EL2, x0


        // Set up VMPIDR_EL2/VPIDR_EL1
        // ---------------------------
        MRS      x0, MIDR_EL1
        MSR      VPIDR_EL2, x0
        MRS      x0, MPIDR_EL1
        MSR      VMPIDR_EL2, x0
        

        // Set VMID
        // ---------
        // Although we are not using stage 2 translation, NS.EL1 still cares
        // about the VMID
        MSR     VTTBR_EL2, xzr


        // Set SCTLRs for EL1/2 to safe values
        // ------------------------------------
        MSR     SCTLR_EL2, xzr
        MSR     SCTLR_EL1, xzr

        mov x0, #0b0101;
        msr spsr_el2, x0;
        
        mov x0, sp;
        msr sp_el1, x0;

        adr x0, .;
        add x0, x0, #16;
        msr elr_el2, x0;
        eret

        nop
        "
    );
}

pub fn initialize_arch() {
    unsafe {
        switch_to_el1();
        let core_id = get_core_id_el1();
        if core_id != 0 {
            halt_core();
        }

        initialize_exception_vector_table_el1();
    }
}

pub fn system_call(call_id: u64, arg: u64) {
    unsafe {
        asm!(
            "mov x10, {call_id}",
            "mov x11, {arg}",
            "svc #0",
            call_id = in(reg) call_id,
            arg = in(reg) arg,
        )
    }
}

unsafe fn get_ec_bits() -> u64 {
    let mut ec_bits: u64;
    asm!("mrs {x}, esr_el1", x = out(reg) ec_bits);
    (ec_bits >> 26) & 0b111111
}

unsafe fn get_far_el1() -> u64 {
    let mut far_el1: u64;
    asm!("mrs {x}, far_el1", x = out(reg) far_el1);
    far_el1
}

enum ExceptionType {
    Unknown,
    Trapped,
    SystemCall,
    DataAbort,
    Other,
}

impl ExceptionType {
    fn from_ec_bits(ec_bits: u64) -> Self {
        if ec_bits == 0b000000 {
            ExceptionType::Unknown
        } else if ec_bits <= 0b010000 {
            ExceptionType::Trapped
        } else if ec_bits == 0b010001 || ec_bits == 0b010101 {
            ExceptionType::SystemCall
        } else if ec_bits == 0b100100 || ec_bits == 0b100101 {
            ExceptionType::DataAbort
        } else {
            ExceptionType::Other
        }
    }
}

fn handle_system_call(syscall_id: u64, arg: u64) {
    info!("System call: id={}, arg={}, ", syscall_id, arg);
}

pub trait DataAbortHandler {
    fn handle_data_abort(&mut self, address: usize) -> bool;
}

pub static mut KERNEL_DATA_ABORT_HANDLER: Option<Box<dyn DataAbortHandler>> = None;
pub static mut PROCESS: Option<Process> = None;

#[no_mangle]
pub unsafe extern "C" fn handle_exception(arg0: u64, arg1: u64, regs: u64) {
    let ec_bits = unsafe { get_ec_bits() };
    let exception_type = ExceptionType::from_ec_bits(ec_bits);
    let spsr_el1 = get_spsr_el1();
    let far_el1 = unsafe { get_far_el1() };

    let from_userspace = spsr_el1 & 0b1111 == 0;

    if from_userspace {
        let reg_slice = unsafe { core::slice::from_raw_parts(regs as *const u64, 31) };
        let mut state = CoreSiteState {
            registers: [0; 31],
            spsr: get_spsr_el1() as u64,
            pc: get_elr_el1(),
            sp: get_sp_el0(),
        };
        for i in 0..31 {
            state.registers[i] = reg_slice[i];
        }

        PROCESS.as_mut().unwrap().site_state = state;

        match exception_type {
            ExceptionType::SystemCall => {
                let syscall_id = arg0;
                let arg = arg1;
                handle_system_call(syscall_id, arg);
                PROCESS.as_ref().unwrap().jump_to_userspace();
            }
            ExceptionType::DataAbort => {
                PROCESS.as_mut().unwrap().on_data_abort(far_el1 as usize);
                PROCESS.as_ref().unwrap().jump_to_userspace();
            }
            _ => {
                panic!("Unhandled exception from userspace");
            }
        }
    } else {
        match exception_type {
            ExceptionType::Unknown => {
                info!("Unknown exception");
            }
            ExceptionType::Trapped => {
                info!("Trapped exception");
            }
            ExceptionType::SystemCall => handle_system_call(arg0, arg1),
            ExceptionType::DataAbort => {
                if let Some(handler) = unsafe { KERNEL_DATA_ABORT_HANDLER.as_mut() } {
                    if handler.handle_data_abort(far_el1 as usize) {
                        return;
                    } else {
                        panic!("Unable to handle data abort at {:#x}", far_el1);
                    }
                } else {
                    panic!("Data abort at {:#x}. No handler provided.", far_el1);
                }
            }
            ExceptionType::Other => {
                info!("Other exception");
            }
        }
    }
}

fn get_spsr_el1() -> usize {
    let spsr_el1: usize;
    unsafe {
        asm!("mrs {x}, spsr_el1", x = out(reg) spsr_el1);
    }
    spsr_el1
}

fn get_elr_el1() -> usize {
    let elr_el1: usize;
    unsafe {
        asm!("mrs {x}, elr_el1", x = out(reg) elr_el1);
    }
    elr_el1
}

fn get_sp_el0() -> usize {
    let sp_el0: usize;
    unsafe {
        asm!("mrs {x}, sp_el0", x = out(reg) sp_el0);
    }
    sp_el0
}

pub fn get_current_el() -> u64 {
    let current_el: u64;
    unsafe {
        asm!("mrs {x}, CurrentEL", x = out(reg) current_el);
    }
    current_el >> 2
}

pub fn start_userspace_execution(addr: usize, sp: usize, ttbr0_el1: usize) -> ! {
    let target_address = SHARED_REGION_BEGIN; // TODO: improve calculation
    unsafe {
        asm!(
            "mov x0, {addr}",
            "mov x1, {sp}",
            "mov x2, {ttbr0_el1}",
            "br      {function_address}",
            addr = in(reg) addr,
            sp = in(reg) sp,
            ttbr0_el1 = in(reg) ttbr0_el1,
            function_address = in(reg) target_address,
            options(noreturn),
        );
    }
}

// https://developer.arm.com/documentation/102374/0102/Procedure-Call-Standard
// X19-X23, X24-X28 are callee-saved registers

core::arch::global_asm!(
    r#"
protect_and_handle_exception:
    sub     sp, sp, #0x108
    stp     x0, x1,   [sp, #0x00];
    stp     x2, x3,   [sp, #0x10];
    stp     x4, x5,   [sp, #0x20];
    stp     x6, x7,   [sp, #0x30];
    stp     x8, x9,   [sp, #0x40];
    stp     x10, x11, [sp, #0x50];
    stp     x12, x13, [sp, #0x60];
    stp     x14, x15, [sp, #0x70];
    stp     x16, x17, [sp, #0x80];
    stp     x18, x19, [sp, #0x90];
    stp     x20, x21, [sp, #0xA0];
    stp     x22, x23, [sp, #0xB0];
    stp     x24, x25, [sp, #0xC0];
    stp     x26, x27, [sp, #0xD0];
    stp     x28, x29, [sp, #0xE0];
    str     x30,      [sp, #0xF0];

    mrs     x0, spsr_el1;
    mrs     x1, elr_el1;
    stp     x0, x1, [sp, #0xF8];
    
    mov     x0, x10;
    mov     x1, x11;
    mov     x2, sp;
    bl      handle_exception;

    ldp     x0, x1, [sp, #0xF8];
    msr     spsr_el1, x0;
    msr     elr_el1, x1;

    ldp     x0, x1,   [sp, #0x00];
    ldp     x2, x3,   [sp, #0x10];
    ldp     x4, x5,   [sp, #0x20];
    ldp     x6, x7,   [sp, #0x30];
    ldp     x8, x9,   [sp, #0x40];
    ldp     x10, x11, [sp, #0x50];
    ldp     x12, x13, [sp, #0x60];
    ldp     x14, x15, [sp, #0x70];
    ldp     x16, x17, [sp, #0x80];
    ldp     x18, x19, [sp, #0x90];
    ldp     x20, x21, [sp, #0xA0];
    ldp     x22, x23, [sp, #0xB0];
    ldp     x24, x25, [sp, #0xC0];
    ldp     x26, x27, [sp, #0xD0];
    ldp     x28, x29, [sp, #0xE0];
    ldr     x30,      [sp, #0xF0];
    add     sp, sp, #0x108;

    eret;


.balign 0x80
aarch64_exception_vector_table:

.balign 0x80
current_el_sp0_sync:
    b       protect_and_handle_exception;
.balign 0x80
current_el_sp0_irq:
    b       protect_and_handle_exception;
.balign 0x80
current_el_sp0_fiq:
    b       protect_and_handle_exception;
.balign 0x80
current_el_sp0_serror:
    b       protect_and_handle_exception;

.balign 0x80
current_el_sp1_sync:
    b       protect_and_handle_exception;
.balign 0x80
current_el_sp1_irq:
    b       protect_and_handle_exception;
.balign 0x80
current_el_sp1_fiq:
    b       protect_and_handle_exception;
.balign 0x80
current_el_sp1_serror:
    b       protect_and_handle_exception;

.balign 0x80
lower_el_aarch64_sync:
    b       protect_and_handle_exception;
.balign 0x80
lower_el_aarch64_irq:
    b       protect_and_handle_exception;
.balign 0x80
lower_el_aarch64_fiq:
    b       protect_and_handle_exception;
.balign 0x80
lower_el_aarch64_serror:
    b       protect_and_handle_exception;

.balign 0x80
lower_el_aarch32_sync:
    b       protect_and_handle_exception;
.balign 0x80
lower_el_aarch32_irq:
    b       protect_and_handle_exception;
.balign 0x80
lower_el_aarch32_fiq:
    b       protect_and_handle_exception;
.balign 0x80
lower_el_aarch32_serror:
    b       protect_and_handle_exception;
"#
);

// TT block entries templates   (L1 and L2, NOT L3)
// Assuming table contents:
// 0 = b01000100 = Normal, Inner/Outer Non-Cacheable
// 1 = b11111111 = Normal, Inner/Outer WB/WA/RA
// 2 = b00000000 = Device-nGnRnE
// .equ TT_S1_FAULT,           0x0
// .equ TT_S1_NORMAL_NO_CACHE, 0x00000000000000401    // Index = 0, AF=1
// .equ TT_S1_NORMAL_WBWA,     0x00000000000000405    // Index = 1, AF=1
// .equ TT_S1_DEVICE_nGnRnE,   0x00600000000000409    // Index = 2, AF=1, PXN=1, UXN=1

// // ------------------------------------------------------------
// // Translation tables
// // ------------------------------------------------------------

//     .section  TT,"ax"
//     .align 12

//     .global tt_l1_base
// tt_l1_base:
//     .fill 4096 , 1 , 0

//     .align 12
//     .global tt_l2_base
// tt_l2_base:
//     .fill 4096 , 1 , 0
