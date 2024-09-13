extern crate alloc;

use core::{
    ops::{Index, IndexMut},
    ptr::*,
};

pub struct MemoryAlignedArray<T> {
    buffer: *mut T,
    capacity: usize,
    count: usize,
    alignment: usize,
}

impl<T> MemoryAlignedArray<T> {
    pub fn new(alignment: usize) -> Self {
        let mut buffer = core::ptr::null_mut();

        let capacity = 1;
        let count = 0;

        let size = capacity * core::mem::size_of::<T>();

        unsafe {
            let layout = core::alloc::Layout::from_size_align(size, alignment).unwrap();
            buffer = alloc::alloc::alloc(layout) as *mut T;
        }
        Self {
            buffer,
            capacity,
            count,
            alignment,
        }
    }

    pub fn expand(&mut self) {
        if self.count < self.capacity {
            return;
        }

        let new_capcity = self.capacity * 2;

        let new_size = new_capcity * core::mem::size_of::<T>();
        let new_buffer = unsafe {
            let layout = core::alloc::Layout::from_size_align(new_size, self.alignment).unwrap();
            alloc::alloc::alloc(layout) as *mut T
        };

        let used_bytes = self.count * core::mem::size_of::<T>();

        unsafe {
            core::ptr::copy(self.buffer, new_buffer, used_bytes);
            let layout = core::alloc::Layout::from_size_align(
                self.capacity * core::mem::size_of::<T>(),
                self.alignment,
            )
            .unwrap();
            alloc::alloc::dealloc(self.buffer as *mut u8, layout);
        }

        self.buffer = new_buffer;
        self.capacity = new_capcity;
    }

    pub fn get(&self, index: usize) -> Option<&T> {
        if index >= self.count {
            return None;
        }
        unsafe { Some(&*self.buffer.add(index)) }
    }

    pub fn get_mut(&mut self, index: usize) -> Option<&mut T> {
        if index >= self.count {
            return None;
        }
        unsafe { Some(&mut *self.buffer.add(index)) }
    }

    pub fn append(&mut self, value: T) {
        self.expand();
        unsafe {
            core::ptr::write(self.buffer.add(self.count), value);
        }
        self.count += 1;
    }

    pub fn len(&self) -> usize {
        self.count
    }

    pub fn get_address(&self) -> *const T {
        self.buffer
    }

    pub fn get_mut_address(&mut self) -> *mut T {
        self.buffer
    }
}

impl<T> Index<usize> for MemoryAlignedArray<T> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        self.get(index).unwrap()
    }
}

impl<T> IndexMut<usize> for MemoryAlignedArray<T> {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        self.get_mut(index).unwrap()
    }
}

impl<T> Drop for MemoryAlignedArray<T> {
    fn drop(&mut self) {
        unsafe {
            let layout = core::alloc::Layout::from_size_align(
                self.count * core::mem::size_of::<T>(),
                self.alignment,
            )
            .unwrap();
            alloc::alloc::dealloc(self.buffer as *mut u8, layout);
        }
    }
}
