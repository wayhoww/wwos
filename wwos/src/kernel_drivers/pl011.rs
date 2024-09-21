use core::ptr::*;

use alloc::vec;

use crate::library::{DeviceTreeNode, MemoryMappingChain};

use super::{Driver, DriverFactory, Serial32Driver};

pub struct PL011Driver {
    addr: *mut u32,
}

impl PL011Driver {
    pub fn new(addr: *mut u32) -> Self {
        Self { addr }
    }
}

impl Driver for PL011Driver {
    fn as_serial32(&self) -> Option<&dyn Serial32Driver> {
        Some(self)
    }
    fn as_serial32_mut(&mut self) -> Option<&mut dyn Serial32Driver> {
        Some(self)
    }
}

impl Serial32Driver for PL011Driver {
    fn write(&mut self, data: u32) {
        unsafe {
            while read_volatile(self.addr.offset(0x18 / 4)) & (1 << 5) != 0 {}
            write_volatile(self.addr, data);
        }
    }

    fn read(&self) -> u32 {
        unsafe {
            while read_volatile(self.addr.offset(0x18 / 4)) & (1 << 4) != 0 {}
            read_volatile(self.addr)
        }
    }
}

pub struct PL011DriverFactory;
impl DriverFactory for PL011DriverFactory {
    fn supported_devices(&self) -> alloc::vec::Vec<&'static str> {
        vec!["arm,pl011"]
    }

    fn instantiate(
        &self,
        device: &DeviceTreeNode,
        parent: Option<&DeviceTreeNode>,
        mapping_chain: &MemoryMappingChain,
    ) -> Option<alloc::boxed::Box<dyn super::Driver>> {
        let parent = parent?;

        let (addr, _size) = device.get_address_size(parent)?;

        let addr = mapping_chain.get_up(addr)?;

        Some(alloc::boxed::Box::new(PL011Driver::new(addr as *mut u32)))
    }
}
