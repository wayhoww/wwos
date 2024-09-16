#![no_std]

use core::ptr::write;
use core::{alloc::Layout, ptr::write_volatile};
use core::fmt::{Write};

struct Uart;

impl core::fmt::Write for Uart {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            unsafe { write_volatile(0xfe201000usize as *mut u8, c as u8) }
        }

        Ok(())
    }
}


#[derive(Clone, Copy, Debug)]
struct ChunkHeader {
    size: usize,  // size is the actual size of this chunk except header. size for alignment is included.
    next: *mut ChunkHeader, // reuse as 'begin of this'
}


pub struct Allocator {
    head: *mut ChunkHeader, // head itself is always empty.
}

impl Allocator {
    pub fn new(address: usize, size: usize) -> Self {
        if size < core::mem::size_of::<ChunkHeader>() {
            panic!("size is too small");
        }

        let head = address as *mut ChunkHeader;
        unsafe {
            (*head).size = 0;
            (*head).next = core::ptr::null_mut();
        }

        if size >= core::mem::size_of::<ChunkHeader>() * 2 {
            let next = (address + core::mem::size_of::<ChunkHeader>()) as *mut ChunkHeader;
            unsafe {
                (*next).size = size - 2 * core::mem::size_of::<ChunkHeader>();
                (*next).next = core::ptr::null_mut();
                (*head).next = next;
            }
        }

        let out = Self {
            head
        };
        out.dump();
        out
    }

    fn dump(&self) {
        let mut uart = Uart;
        let mut current = self.head;

        write!(uart, "dump: \n").unwrap();

        let mut cnt = 0;

        while !current.is_null() {
            let chunk = unsafe { &*current };
            write!(uart, "{:p} size: {}, next={:p}\n", current, chunk.size, chunk.next).unwrap();
            current = chunk.next;

            cnt += 1;
            if cnt >= 20 {
                write!(uart, "...\n").unwrap();
                break;
            }
        }

        write!(uart, "\n").unwrap();
    }

    fn find_chunk(&mut self, size: usize, align: usize) -> Option<(*mut ChunkHeader, *mut ChunkHeader)> {
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
        writeln!(Uart, "allocate size = {}, alignment={}", layout.size(), layout.align()).unwrap();

        // TODO check align is power of 2
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
                (*p_new_chunk_header).size = chunk.size - (end - start) - core::mem::size_of::<ChunkHeader>();
                (*p_new_chunk_header).next = chunk.next;

                writeln!(Uart, "set next of {:p} to {:p}", prev, (*prev).next).unwrap();

                // writeln!(Uart, "set next of {:p} to {:p}", p_new_chunk_header, new_chunk_header.next).unwrap();
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

            let p_new_chunk_header = (aligned_start - core::mem::size_of::<ChunkHeader>()) as *mut ChunkHeader;
            unsafe { *p_new_chunk_header = new_chunk_header; }
        }

        writeln!(Uart, "allocated {:x}", aligned_start).unwrap();
        self.dump();

        Some(aligned_start)
    }

    pub fn deallocate(&mut self, address: usize) {
        writeln!(Uart, "deallocate {:x}", address).unwrap();

        if address == 0 {
            return;
        }

        let p_chunk = (address - core::mem::size_of::<ChunkHeader>()) as *mut ChunkHeader;
        let chunk = unsafe { *p_chunk };
        let new_p_chunk = chunk.next; // reused as 'begin of this'

        let mut prev = self.head;
        let mut current = unsafe { (*prev).next };

        while !prev.is_null() {
            // if new_p_chunk is greater than prev, then insert here.

            // prev < new_p_chunk < current
            if prev < new_p_chunk && new_p_chunk < current {

                // ensure no overlap
                let end_of_prev = prev as usize + core::mem::size_of::<ChunkHeader>() + unsafe {(*prev).size};
                let end_of_new_p_chunk = new_p_chunk as usize + core::mem::size_of::<ChunkHeader>() + chunk.size;

                if end_of_prev > new_p_chunk as usize || end_of_new_p_chunk > current as usize {
                    panic!("memory corruption");
                }

                let connected_with_prev = ((prev as usize) + core::mem::size_of::<ChunkHeader>() + unsafe {(*prev).size} == new_p_chunk as usize) && (prev != self.head);
                let connected_with_current = new_p_chunk as usize + core::mem::size_of::<ChunkHeader>() + chunk.size == current as usize;
            
                assert!(!(current.is_null() && connected_with_current));

                if connected_with_prev && connected_with_current {
                    unsafe {
                        (*prev).size += 2 * core::mem::size_of::<ChunkHeader>() + chunk.size + (*current).size;
                        (*prev).next = (*current).next;
                    }
                } else if connected_with_prev {
                    unsafe {
                        (*prev).size += core::mem::size_of::<ChunkHeader>() + chunk.size;
                        (*prev).next = current;
                    }
                } else if connected_with_current {
                    unsafe {
                        (*p_chunk).size += core::mem::size_of::<ChunkHeader>() + (*current).size;
                        (*p_chunk).next = (*current).next;
                        (*prev).next = p_chunk;
                    }
                } else {
                    unsafe {
                        (*p_chunk).next = current;
                        (*prev).next = p_chunk;
                    }

                }

                self.dump();
                return;
            }

            prev = current;
            current = unsafe { (*current).next };
        }

        panic!("memory corruption");
    }
}


