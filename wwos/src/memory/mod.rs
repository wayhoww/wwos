// 1. some special area for special memory
// 2. large linear area, alloc on demand

mod kalloc;

pub use kalloc::KERNEL_MEMORY_ALLOCATOR;

#[derive(Clone, Debug)]
pub enum MemoryType {
    Device,
    Normal,
}

#[derive(Clone, Debug)]
pub enum MemoryPermission {
    KernelRWX,
    KernelRwUserRWX,
}

#[derive(Clone, Debug)]
pub struct MemoryBlockAttributes {
    pub permission: MemoryPermission,
    pub mtype: MemoryType,
}

#[derive(Clone, Debug)]
pub struct MemoryBlock {
    pub virtual_address: usize,
    pub physical_address: usize,
    pub attr: MemoryBlockAttributes,
}
