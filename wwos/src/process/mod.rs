use core::{cell::RefCell, cmp::min};

use alloc::{rc::Rc, vec::Vec};

use crate::arch::{CoreSiteState, TranslationTable, PAGE_SIZE, USER_BINARY_BEGIN};
use crate::KERNEL_MEMORY_BLOCKS;
use crate::{
    memory::{MemoryBlock, MemoryBlockAttributes, MemoryPermission, MemoryType},
    PhysicalMemoryPageAllocator,
};

pub struct Process {
    pub site_state: CoreSiteState,
    pub translation_table: TranslationTable,
    pub physical_allocator: Rc<RefCell<PhysicalMemoryPageAllocator>>,
}

impl Process {
    pub fn from_binary_slice(
        binary: &[u8],
        physical_allocator: Rc<RefCell<PhysicalMemoryPageAllocator>>,
    ) -> Process {
        let page_count = (binary.len() + PAGE_SIZE - 1) / PAGE_SIZE;
        let mut pages = Vec::new();

        for i in 0..page_count {
            let physical_address = physical_allocator.borrow_mut().alloc().unwrap();
            let virtual_address = USER_BINARY_BEGIN + i * PAGE_SIZE;
            let page = MemoryBlock {
                virtual_address,
                physical_address,
                attr: MemoryBlockAttributes {
                    permission: MemoryPermission::KernelRwUserRWX,
                    mtype: MemoryType::Normal,
                },
            };
            pages.push(page);

            let start = i * PAGE_SIZE;
            let end = min((i + 1) * PAGE_SIZE, binary.len());
            let slice = &binary[start..end];
            unsafe {
                core::ptr::copy(slice.as_ptr(), physical_address as *mut u8, slice.len());
            }
        }

        let mut translation_table = TranslationTable::from_memory_pages(&pages);

        for page in unsafe { KERNEL_MEMORY_BLOCKS.iter() } {
            translation_table.add_block(page);
        }

        Process {
            site_state: CoreSiteState::default(),
            translation_table: translation_table,
            physical_allocator: physical_allocator,
        }
    }

    pub fn jump_to_userspace(&self) {
        self.translation_table.activate();
        self.site_state.jump_to_userspace();
    }

    pub fn on_data_abort(&mut self, addr: usize) {
        // self
        let page = MemoryBlock {
            virtual_address: addr,
            physical_address: self.physical_allocator.borrow_mut().alloc().unwrap(),
            attr: MemoryBlockAttributes {
                permission: MemoryPermission::KernelRwUserRWX,
                mtype: MemoryType::Normal,
            },
        };

        self.translation_table.add_block(&page);
        self.translation_table.activate();
    }
}
