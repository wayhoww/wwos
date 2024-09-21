use core::{alloc::Layout, arch::asm};

use alloc::{alloc::{alloc, dealloc}, boxed::Box, vec::Vec};

use crate::memory::{MemoryBlock, MemoryPermission};


struct TranslationTableNode {
    level: usize,
    table: *mut u64,
    next_level_tables: Vec<Option<TranslationTableNode>>,
}

pub struct TranslationTable {
    root: Box<TranslationTableNode>,
}

pub const PAGE_SIZE: usize = 4096;

impl Drop for TranslationTableNode {
    fn drop(&mut self) {
        unsafe {
            dealloc(
                self.table as *mut u8,
                Layout::from_size_align(PAGE_SIZE, PAGE_SIZE).unwrap(),
            )
        };
    }
}

impl TranslationTableNode {
    fn new(level: usize) -> Self {
        let table =
            unsafe { alloc(Layout::from_size_align(PAGE_SIZE, PAGE_SIZE).unwrap()) as *mut u64 };
        for i in 0..512 {
            unsafe { table.offset(i).write(0u64) };
        }

        Self {
            level,
            table,
            next_level_tables: (0..512).map(|_| None).collect(),
        }
    }

    fn add_node(&mut self, page: &MemoryBlock) {
        let index = (page.virtual_address >> (39 - self.level * 9)) & ((1 << 9) - 1);

        assert!(index < 512);

        if self.level < 3 {
            if self.next_level_tables[index].is_none() {
                self.next_level_tables[index] =
                    Some(TranslationTableNode::new(self.level + 1));
                unsafe {
                    let addr = self.table.offset(index as isize);
                    *addr = (self.next_level_tables[index]
                        .as_ref()
                        .unwrap()
                        .get_table_address() as u64)
                        | 3;
                }
            }

            self.next_level_tables[index]
                .as_mut()
                .unwrap()
                .add_node(page);
        } else {
            let mut block_template = 0x0060000000000040Bu64;
            
            match page.attr.permission {
                MemoryPermission::KernelRWX => {}
                MemoryPermission::KernelRXUserRX => { block_template |= 0b11 << 6; }
                MemoryPermission::KernelRwUserRWX => { block_template |= 0b01 << 6; }
            }
            
            unsafe {
                let addr = self.table.offset(index as isize);
                *addr = block_template | (page.physical_address as u64);
            }
        }
    }

    fn get_table_address(&self) -> usize {
        self.table as usize
    }
}

impl TranslationTable {
    pub fn get_table_address(&self) -> usize {
        self.root.get_table_address()
    }

    pub fn from_memory_pages(pages: &[MemoryBlock]) -> Self {
        let mut root = Box::new(TranslationTableNode::new(1));

        for page in pages {
            root.add_node(&page);
        }

        Self { root }
    }

    pub fn add_block(&mut self, block: &MemoryBlock) {
        self.root.add_node(block);
    }

    pub fn activate(&self) {
        let addr = self.get_table_address();
        unsafe { Self::activate_address(addr) };
    }

    pub unsafe fn activate_address(addr: usize) {
        let mut _tmp: usize;

        unsafe {
            asm!(
                r#"

                DSB      SY
                ISB

                // Set the Base address
                // ---------------------
                MSR      TTBR0_EL1, {addr}


                // Set up memory attributes
                // -------------------------
                // This equates to:
                // 0 = b01000100 = Normal, Inner/Outer Non-Cacheable
                // 1 = b11111111 = Normal, Inner/Outer WB/WA/RA
                // 2 = b00000000 = Device-nGnRnE
                MOV      {tmp}, #0x000000000000FF44
                MSR      MAIR_EL1, {tmp}


                // Set up TCR_EL1
                // ---------------
                MOV      {tmp}, #0x19                           // T0SZ=0b011001 Limits VA space to 39 bits, translation starts @ l1
                ORR      {tmp}, {tmp}, #(0x1 << 8)              // IGRN0=0b01    Walks to TTBR0 are Inner WB/WA
                ORR      {tmp}, {tmp}, #(0x1 << 10)             // OGRN0=0b01    Walks to TTBR0 are Outer WB/WA
                ORR      {tmp}, {tmp}, #(0x3 << 12)             // SH0=0b11      Inner Shareable
                ORR      {tmp}, {tmp}, #(0x1 << 23)             // EPD1=0b1      Disable table walks from TTBR1
                                                                // TBI0=0b0
                                                                // TG0=0b00      4KB granule for TTBR0 (Note: Different encoding to TG0)
                                                                // A1=0          TTBR0 contains the ASID
                                                                // AS=0          8-bit ASID
                                                                // IPS=0         32-bit IPA space
                MSR      TCR_EL1, {tmp}

                // NOTE: We don't need to set up T1SZ/TBI1/ORGN1/IRGN1/SH1, as we've set EPD==1 (disabling walks from TTBR1)


                // Ensure changes to system register are visible before MMU enabled
                ISB


                // Invalidate TLBs
                // ----------------
                TLBI     VMALLE1
                DSB      SY
                ISB

                // Enable MMU
                // -----------
                MOV      {tmp}, #(1 << 0)                       // M=1           Enable the stage 1 MMU
                ORR      {tmp}, {tmp}, #(1 << 2)                // C=1           Enable data and unified caches
                ORR      {tmp}, {tmp}, #(1 << 12)               // I=1           Enable instruction fetches to allocate into unified caches
                                                                // A=0           Strict alignment checking disabled
                                                                // SA=0          Stack alignment checking disabled
                                                                // WXN=0         Write permission does not imply XN
                                                                // EE=0          EL3 data accesses are little endian
                MSR      SCTLR_EL1, {tmp}
                ISB

                "#,
                addr = in(reg) addr,
                tmp = out(reg) _tmp,
            );
        }
    }
}
