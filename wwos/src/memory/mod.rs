

// 1. some special area for special memory
// 2. large linear area, alloc on demand

mod kalloc;

pub use kalloc::KERNEL_MEMORY_ALLOCATOR;

use core::{alloc::Layout, arch::asm};

use alloc::{alloc::alloc, boxed::Box, vec::Vec};
use alloc::vec;

use crate::library::MemoryAlignedArray;

pub enum MemoryType {
    Device,
    Normal,
}

pub struct MemoryPage {
    pub virtual_address: usize,
    pub physical_address: usize,
    pub user_readable: bool,
    pub user_writable: bool,
    pub kernel_readable: bool,
    pub kernel_writable: bool,
    pub memory_type: MemoryType,
}


unsafe fn alloc_memory_for_page_table_aarch64() -> *mut u64 {
    // start: is where virtual address are identically mapped to physical address

    static mut START: usize = 0x20000000;
    
    let addr = START;
    START += 4096;

    if START >= 0x30000000 {
        panic!("Out of memory");
    }

    // clear
    for i in 0..512 {
        let addr = addr as *mut u64;
        addr.offset(i).write(0);
    }

    addr as *mut u64
}

struct TranslationTableNodeAarch64 {
    level: usize,
    table: *mut u64,
    next_level_tables: Vec<Option<TranslationTableNodeAarch64>>,
}

impl Drop for TranslationTableNodeAarch64 {
    fn drop(&mut self) {
        // TODO(ww): free memory
    }
}

pub struct TranslationTableAarch64 {
    root: Box<TranslationTableNodeAarch64>,
}

impl TranslationTableNodeAarch64 {
    fn new(level: usize) -> Self {
        let table = unsafe {
            alloc_memory_for_page_table_aarch64()
        };


        Self {
            level,
            table,
            next_level_tables: (0..512)
                .map(|_| None)
                .collect(),
        }
    }
    
    fn add_node(&mut self, page: &MemoryPage) {
        let index = (page.virtual_address >> (39 - self.level * 9)) & ((1 << 9) - 1);
        
        if self.level < 3 {
            if self.next_level_tables[index].is_none() {
                self.next_level_tables[index] = Some(TranslationTableNodeAarch64::new(self.level + 1));
                unsafe {
                    let addr = self.table.offset(index as isize);
                    *addr = (self.next_level_tables[index].as_ref().unwrap().get_table_address() as u64) | 3;
                }
            }

            self.next_level_tables[index].as_mut().unwrap().add_node(page);
        } else {
            let block_template = 0x0060000000000040Bu64;
            unsafe {
                let addr = self.table.offset(index as isize);
                *addr = block_template | (page.physical_address as u64) | (3 << 8);
            }
        }

    }

    fn get_table_address(&self) -> usize {
        self.table as usize
    }
}

impl TranslationTableAarch64 {
    fn from_memory_pages(pages: &[MemoryPage]) -> Self {
        let mut root = Box::new(TranslationTableNodeAarch64::new(1));

        for page in pages {
            root.add_node(&page);
        }

        Self { root }
    }

    fn get_table_address(&self) -> usize {
        self.root.get_table_address()
    }
}


pub fn set_translation_table(pages: &[MemoryPage]) {
    let translation_table = TranslationTableAarch64::from_memory_pages(pages);
    let addr = translation_table.get_table_address();
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
    MOV      {tmp}, #0x19                        // T0SZ=0b011001 Limits VA space to 39 bits, translation starts @ l1
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
    MOV      {tmp}, #(1 << 0)                     // M=1           Enable the stage 1 MMU
    ORR      {tmp}, {tmp}, #(1 << 2)                 // C=1           Enable data and unified caches
    ORR      {tmp}, {tmp}, #(1 << 12)                // I=1           Enable instruction fetches to allocate into unified caches
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