use core::arch::asm;


pub const USER_MEMORY_BEGIN: usize = 0x0000_0001_0000_0000;
pub const USER_BINARY_BEGIN: usize = USER_MEMORY_BEGIN;

pub struct CoreSiteState {
    pub registers: [u64; 31], // x0 - x30
    pub spsr: u64,
    pub pc: usize,
    pub sp: usize,
}

impl Default for CoreSiteState {
    fn default() -> Self {
        CoreSiteState {
            registers: [0; 31],
            spsr: 0,
            pc: USER_BINARY_BEGIN,
            sp: USER_MEMORY_BEGIN + 1024 * 1024, // todo
        }
    }
}


impl CoreSiteState {
    pub fn jump_to_userspace(&self) -> ! {
        let register_slice = &self.registers[..];
        let register_addr = register_slice.as_ptr() as u64;

        // step 1, set spsr_el1, elr_el1, and sp_el0
        unsafe {
            asm!("msr spsr_el1, {0}", in(reg) self.spsr);
            asm!("msr elr_el1, {0}", in(reg) self.pc);
            asm!("msr sp_el0, {0}", in(reg) self.sp);
        }

        // step 2, set general purpose registers
        unsafe {
            asm!(
                r#"
                // ldr x0, =stack_top;
                // mov sp, x0;

                mov x0, {regs}
                ldr x1, [x0, #8]
                ldr x2, [x0, #16]
                ldr x3, [x0, #24]
                ldr x4, [x0, #32]
                ldr x5, [x0, #40]
                ldr x6, [x0, #48]
                ldr x7, [x0, #56]
                ldr x8, [x0, #64]
                ldr x9, [x0, #72]
                ldr x10, [x0, #80]
                ldr x11, [x0, #88]
                ldr x12, [x0, #96]
                ldr x13, [x0, #104]
                ldr x14, [x0, #112]
                ldr x15, [x0, #120]
                ldr x16, [x0, #128]
                ldr x17, [x0, #136]
                ldr x18, [x0, #144]
                ldr x19, [x0, #152]
                ldr x20, [x0, #160]
                ldr x21, [x0, #168]
                ldr x22, [x0, #176]
                ldr x23, [x0, #184]
                ldr x24, [x0, #192]
                ldr x25, [x0, #200]
                ldr x26, [x0, #208]
                ldr x27, [x0, #216]
                ldr x28, [x0, #224]
                ldr x29, [x0, #232]
                ldr x30, [x0, #240]
                ldr x0, [x0, #0]

                eret
                "#, 
                regs = in(reg) register_addr,
                options(noreturn),
            );
        }
    }
}