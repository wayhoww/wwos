struct DummyAllocator;
unsafe impl core::alloc::GlobalAlloc for DummyAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        const SIZE: usize = 1024 * 1024 * 4;
        static mut HEAP: [u8; SIZE] = [0; SIZE];
        static mut N_USED: usize = 0;

        let align = layout.align();
        let size = layout.size();

        let ptr_begin = core::ptr::addr_of!(HEAP) as *const u8 as usize + N_USED;
        let ptr_aligned = (ptr_begin + align - 1) & !(align - 1);

        N_USED = ptr_aligned - core::ptr::addr_of!(HEAP) as *const u8 as usize + size;

        ptr_aligned as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: core::alloc::Layout) {
    }
}

#[global_allocator]
static GLOBAL: DummyAllocator = DummyAllocator;

