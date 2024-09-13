use core::ptr::*;

use alloc::vec;

use crate::library::{DeviceTreeNode, MemoryAlignedArray};

use super::Driver;

pub struct MailboxPropertiesBuffer {
    buffer: MemoryAlignedArray<u32>,
}

impl MailboxPropertiesBuffer {
    pub fn new() -> Self {
        let mut buffer = MemoryAlignedArray::new(16);
        buffer.append(0);
        buffer.append(0);
        Self { buffer }
    }

    pub fn add_tag(&mut self, tag_identifier: u32, values: &[u32]) -> usize {
        let offset = self.buffer.len();

        let size = values.len();
        self.buffer.append(tag_identifier);
        self.buffer
            .append((size as u32) * (core::mem::size_of::<u32>() as u32));
        self.buffer.append(0);
        for value in values {
            self.buffer.append(*value);
        }

        offset
    }

    pub fn finish(&mut self) {
        self.buffer.append(0);
        self.buffer[0] = (self.buffer.len() * core::mem::size_of::<u32>()) as u32;
    }

    pub fn get_buffer_address(&self) -> *const u32 {
        self.buffer.get_address()
    }

    pub fn get_mut_buffer_address(&mut self) -> *mut u32 {
        self.buffer.get_mut_address()
    }

    pub fn get_buffer(&self) -> &MemoryAlignedArray<u32> {
        return &self.buffer;
    }
}

enum MailboxStatus {
    Full = 0x80000000,
    Empty = 0x40000000,
}

pub struct Mailbox {
    addr: *mut u32,
}

const MAILBOX_OFFSET_DATA: isize = 0x00;
const MAILBOX_OFFSET_PEEK: isize = 0x10;
const MAILBOX_OFFSET_SENDER: isize = 0x14;
const MAILBOX_OFFSET_STATUS: isize = 0x18;
const MAILBOX_OFFSET_CONFIG: isize = 0x1c;

impl Mailbox {
    pub fn new(address: *mut u32) -> Self {
        Self { addr: address }
    }

    fn read(&self, channel: u32) -> u32 {
        loop {
            while unsafe { read_volatile(self.addr.offset(MAILBOX_OFFSET_STATUS)) }
                & MailboxStatus::Empty as u32
                != 0
            {}
            let data = unsafe { read_volatile(self.addr.offset(MAILBOX_OFFSET_DATA)) };
            if data & 0xf == channel {
                return data >> 4;
            }
        }
    }

    fn write(&self, data: u32, channel: u32) {
        let addr_status = unsafe { self.addr.byte_offset(MAILBOX_OFFSET_STATUS) };

        while unsafe { read_volatile(addr_status) } & MailboxStatus::Full as u32 != 0 {}
        let assembled = (data << 4) | (channel & 0xf);
        unsafe { write_volatile(self.addr.offset(MAILBOX_OFFSET_DATA), assembled) };
    }

    pub fn write_properties(&self, properties: &MailboxPropertiesBuffer, channel: u32) {
        let addr64 = properties.get_buffer_address() as usize;
        let addr32 = addr64 as u32;
        self.write(addr32 >> 4, channel);
    }
}

impl Driver for Mailbox {
    fn get_memory_mapping(&self) -> Option<crate::library::MemoryMapping> {
        None
    }
}

pub struct MailboxDriverFactory;
impl super::DriverFactory for MailboxDriverFactory {
    fn supported_devices(&self) -> alloc::vec::Vec<&'static str> {
        return vec!["brcm,bcm2711-vchiq"];
    }

    fn instantiate(
        &self,
        device: &crate::library::DeviceTreeNode,
        parent: Option<&DeviceTreeNode>,
        mapping_chain: &crate::library::MemoryMappingChain,
    ) -> Option<alloc::boxed::Box<dyn super::Driver>> {
        let reg = device.find_property_by_name("reg")?;
        let lower_addr = u32::from_be_bytes(reg[0..4].try_into().unwrap()) as *mut u32;
        let upper_addr = mapping_chain.get_up(lower_addr as usize)?;
        let addr = upper_addr as *mut u32;
        Some(alloc::boxed::Box::new(Mailbox::new(addr)))
    }
}
