use core::arch::asm;

use log::info;

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

    asm!("
        mov x0, #(0b1 << 31);
        msr hcr_el2, x0;

        mov {tmp}, #0b0101;
        msr spsr_el2, {tmp};
        
        mov {tmp}, sp;
        msr sp_el1, {tmp};

        adr {tmp}, .;
        add {tmp}, {tmp}, #16;
        msr elr_el2, {tmp};
        eret

        nop
        ", 
        tmp = out(reg) _tmp,
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

enum ExceptionType {
    Unknown,
    Trapped,
    SystemCall,
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
        } else {
            ExceptionType::Other
        }
    }
}

fn handle_system_call(syscall_id: u64, arg: u64) {
    info!("System call: id={}, arg={}, ", syscall_id, arg);
    // print current stack pointer
    let sp: usize;
    unsafe {
        asm!("mov {sp}, sp", sp = out(reg) sp);
    }
    info!("Current stack pointer: {:#x}", sp);
}

#[no_mangle]
fn handle_exception(arg0: u64, arg1: u64) {
    let ec_bits = unsafe { get_ec_bits() };
    let exception_type = ExceptionType::from_ec_bits(ec_bits);

    match exception_type {
        ExceptionType::Unknown => {
            info!("Unknown exception");
        }
        ExceptionType::Trapped => {
            info!("Trapped exception");
        }
        ExceptionType::SystemCall => handle_system_call(arg0, arg1),
        ExceptionType::Other => {
            info!("Other exception");
        }
    }
}

pub fn get_current_el() -> u64 {
    let current_el: u64;
    unsafe {
        asm!("mrs {x}, CurrentEL", x = out(reg) current_el);
    }
    current_el >> 2
}

pub unsafe fn start_userspace_execution(addr: usize) -> ! {
    let mut _tmp: usize;

    asm!(
        r#"
            ldr {tmp}, =el0_stack_top;
            msr sp_el0, {tmp};
            msr elr_el1, {addr};
            eret;    
        "#,
        addr = in(reg) addr,
        tmp = out(reg) _tmp,
    );

    panic!("Unreachable");
}

// https://developer.arm.com/documentation/102374/0102/Procedure-Call-Standard
// X19-X23, X24-X28 are callee-saved registers

core::arch::global_asm!(
    r#"
protect_and_handle_exception:
    sub     sp, sp, #168
    stp     x0, x1, [sp, #0];
    stp     x2, x3, [sp, #16];
    stp     x4, x5, [sp, #32];
    stp     x6, x7, [sp, #48];
    stp     x8, x9, [sp, #64];
    stp     x10, x11, [sp, #96];
    stp     x12, x13, [sp, #112];
    stp     x14, x15, [sp, #128];
    stp     x16, x17, [sp, #144];
    stp     x18, x29, [sp, #160];
    str     x30, [sp, #168];

    mrs     x0, spsr_el1;
    mrs     x1, elr_el1;
    stp     x0, x1, [sp, #176];
    
    mov     x0, x10;
    mov     x1, x11;
    bl      handle_exception;

    ldp     x0, x1, [sp, #176];
    msr     spsr_el1, x0;
    msr     elr_el1, x1;

    ldp     x0, x1, [sp, #0];
    ldp     x2, x3, [sp, #16];
    ldp     x4, x5, [sp, #32];
    ldp     x6, x7, [sp, #48];
    ldp     x8, x9, [sp, #64];
    ldp     x10, x11, [sp, #96];
    ldp     x12, x13, [sp, #112];
    ldp     x14, x15, [sp, #128];
    ldp     x16, x17, [sp, #144];
    ldp     x18, x29, [sp, #160];
    ldr     x30, [sp, #168];
    add     sp, sp, #168;

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
