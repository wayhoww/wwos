#![no_std]

use core::{alloc::Layout, fmt::Write, ptr::write_volatile};

#[derive(Clone, Copy, Debug)]
struct ChunkHeader {
    size: usize, // size is the actual size of this chunk except header. size for alignment is included.
    next: *mut ChunkHeader, // reuse as 'begin of this'
}

pub struct Allocator {
    head: *mut ChunkHeader,
    begin: usize,
    size: usize,
}

const SIZE_OF_HEADER: usize = core::mem::size_of::<ChunkHeader>();

struct Uart;

impl Write for Uart {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            unsafe { write_volatile(0x0900_0000 as *mut u32, c as u32) };
        }
        Ok(())
    }
}

impl Allocator {
    pub fn new(address: usize, size: usize) -> Self {
        if size < core::mem::size_of::<ChunkHeader>() {
            panic!("size is too small");
        }

        let p_head = address as *mut ChunkHeader;
        let p_next = (address + SIZE_OF_HEADER) as *mut ChunkHeader;

        let head = unsafe { &mut *p_head };
        head.size = 0;
        head.next = p_next;

        let next = unsafe { &mut *p_next };
        next.size = size - SIZE_OF_HEADER * 2;
        next.next = core::ptr::null_mut();

        Self {
            head: p_head,
            begin: address,
            size,
        }
    }

    fn dump(&self) {
        let mut current = self.head;

        while !current.is_null() {
            let chunk = unsafe { &*current };
            current = chunk.next;
        }
    }

    fn find_chunk(
        &mut self,
        size: usize,
        align: usize,
    ) -> Option<(*mut ChunkHeader, *mut ChunkHeader)> {
        let mut prev = self.head;
        let mut current = unsafe { (*prev).next };

        while !current.is_null() {
            let chunk = unsafe { &*current };

            let start = current as usize + core::mem::size_of::<ChunkHeader>();
            let aligned_start = (start + align - 1) & !(align - 1);
            let end = aligned_start + size;

            if chunk.size >= end - start {
                return Some((prev, current));
            }

            prev = current;
            current = chunk.next;
        }
        None
    }

    pub fn allocate(&mut self, layout: Layout) -> Option<usize> {
        let size = layout.size();
        let align = layout.align();
        let size = (size + 7) & !7;
        let align = (align + 7) & !7;

        let (prev, current) = self.find_chunk(size, align)?;

        let chunk = unsafe { *current };
        let start = current as usize + core::mem::size_of::<ChunkHeader>();
        let aligned_start = (start + align - 1) & !(align - 1);
        let end = aligned_start + size;

        if chunk.size > end - start + core::mem::size_of::<ChunkHeader>() {
            unsafe {
                // the next chunk of allocated chunk

                let p_new_chunk_header = end as *mut ChunkHeader;
                (*prev).next = p_new_chunk_header;
                (*p_new_chunk_header).size =
                    chunk.size - (end - start) - core::mem::size_of::<ChunkHeader>();
                (*p_new_chunk_header).next = chunk.next;
            }
        } else {
            unsafe {
                (*prev).next = chunk.next;
            }
        }

        {
            // allocated chunk
            let new_chunk_header = ChunkHeader {
                size: end - start,
                next: current, // reuse as 'begin of this'
            };

            let p_new_chunk_header =
                (aligned_start - core::mem::size_of::<ChunkHeader>()) as *mut ChunkHeader;
            unsafe {
                *p_new_chunk_header = new_chunk_header;
            }

        }

        Some(aligned_start)
    }

    pub fn deallocate(&mut self, address: usize) {
        if address == 0 {
            return;
        }
        self.dump();

        let p_chunk = (address - core::mem::size_of::<ChunkHeader>()) as *mut ChunkHeader;
        let chunk = unsafe { *p_chunk };

        let new_p_chunk = chunk.next; // reused as 'begin of this'

        let mut prev = self.head;
        let mut current = unsafe { (*prev).next };

        while !prev.is_null() {
            if prev < new_p_chunk && (new_p_chunk < current || current.is_null()) {
                // ensure no overlap
                let end_of_prev =
                    prev as usize + core::mem::size_of::<ChunkHeader>() + unsafe { (*prev).size };
                let end_of_new_p_chunk =
                    new_p_chunk as usize + core::mem::size_of::<ChunkHeader>() + chunk.size;

                if end_of_prev > new_p_chunk as usize
                    || (!current.is_null() && end_of_new_p_chunk > current as usize)
                {
                    panic!("memory corruption");
                }

                let connected_with_prev = ((prev as usize)
                    + core::mem::size_of::<ChunkHeader>()
                    + unsafe { (*prev).size }
                    == new_p_chunk as usize)
                    && (prev != self.head);
                let connected_with_current =
                    (new_p_chunk as usize + core::mem::size_of::<ChunkHeader>() + chunk.size
                        == current as usize)
                        && !current.is_null();

                assert!(!(current.is_null() && connected_with_current));

                if connected_with_prev && connected_with_current {
                    unsafe {
                        (*prev).size +=
                            2 * core::mem::size_of::<ChunkHeader>() + chunk.size + (*current).size;
                        (*prev).next = (*current).next;
                    }
                } else if connected_with_prev {
                    unsafe {
                        (*prev).size += core::mem::size_of::<ChunkHeader>() + chunk.size;
                        (*prev).next = current;
                    }
                } else if connected_with_current {
                    unsafe {
                        (*new_p_chunk).size += core::mem::size_of::<ChunkHeader>() + (*current).size;
                        (*new_p_chunk).next = (*current).next;
                        (*prev).next = new_p_chunk;
                    }
                } else {
                    unsafe {
                        (*new_p_chunk).next = current;
                        (*new_p_chunk).size = chunk.size;
                        (*prev).next = new_p_chunk;
                    }
                }

                return;
            }

            prev = current;
            current = unsafe { (*current).next };
        }

        panic!("memory corruption");
    }

    fn get_tail(&self) -> *mut ChunkHeader {
        let mut prev = self.head;
        let mut current = unsafe { (*prev).next };

        while !current.is_null() {
            prev = current;
            current = unsafe { (*current).next };
        }

        prev
    }

    pub unsafe fn extend(&mut self, new_end: usize) {
        let p_tail = self.get_tail();
        let a_tail = p_tail as usize;
        let tail = &mut *p_tail;

        if a_tail + tail.size + SIZE_OF_HEADER == self.begin + self.size {
            // contiguous, just extend
            tail.size += new_end - (self.begin + self.size);
        } else {
            // not contiguous, add new chunk
            let p_new_tail = (self.begin + self.size) as *mut ChunkHeader;
            let new_tail = &mut *p_new_tail;
            new_tail.size = new_end - (self.begin + self.size) - SIZE_OF_HEADER;
            new_tail.next = core::ptr::null_mut();

            tail.next = p_new_tail;
        }

        self.size = new_end - self.begin;
    }

    pub fn get_used_address_upperbound(&self) -> usize {
        // if tail + tail.size + size_of_header == self.begin + self.size , return p_tail + size_of_header
        // else return begin + size

        let p_tail = self.get_tail();
        let a_tail = p_tail as usize;
        let tail = unsafe { &*p_tail };

        if a_tail + tail.size + SIZE_OF_HEADER == self.begin + self.size {
            a_tail + SIZE_OF_HEADER
        } else {
            self.begin + self.size
        }
    }
}
