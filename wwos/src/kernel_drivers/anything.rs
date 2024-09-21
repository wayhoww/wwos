// linux,dummy-virt

use alloc::vec;

use crate::library::{DeviceTreeNode, MemoryMapping};

use super::{Driver, MemoryDriver};

pub struct DummyVirtDriver;
impl Driver for DummyVirtDriver {
    fn get_memory_mapping(&self) -> Option<crate::library::MemoryMapping> {
        None
    }
}

pub struct DummyVirtDriverFactory;
impl super::DriverFactory for DummyVirtDriverFactory {
    fn supported_devices(&self) -> alloc::vec::Vec<&'static str> {
        vec!["linux,dummy-virt"]
    }

    fn instantiate(
        &self,
        _device: &crate::library::DeviceTreeNode,
        _parent: Option<&DeviceTreeNode>,
        _mapping_chain: &crate::library::MemoryMappingChain,
    ) -> Option<alloc::boxed::Box<dyn super::Driver>> {
        Some(alloc::boxed::Box::new(DummyVirtDriver))
    }
}

pub struct Bcm2711Driver;
impl Driver for Bcm2711Driver {
    fn get_memory_mapping(&self) -> Option<crate::library::MemoryMapping> {
        let mut mapping = MemoryMapping::new();

        mapping.add(0xfc00_0000, 0x7c00_0000, 0x400_0000); // peripherals
        mapping.add(0x0_0000_0000, 0x0_0000_0000, 0x4_0000_0000); // memory
        Some(mapping)
    }
}

pub struct Bcm2711DriverFactory;
impl super::DriverFactory for Bcm2711DriverFactory {
    fn supported_devices(&self) -> alloc::vec::Vec<&'static str> {
        vec!["brcm,bcm2711"]
    }

    fn instantiate(
        &self,
        _device: &DeviceTreeNode,
        _parent: Option<&DeviceTreeNode>,
        _mapping_chain: &crate::library::MemoryMappingChain,
    ) -> Option<alloc::boxed::Box<dyn super::Driver>> {
        Some(alloc::boxed::Box::new(Bcm2711Driver))
    }
}

pub struct SimpleBusDriver;
impl Driver for SimpleBusDriver {
    fn get_memory_mapping(&self) -> Option<crate::library::MemoryMapping> {
        None
    }
}

pub struct SimpleBusDriverFactory;
impl super::DriverFactory for SimpleBusDriverFactory {
    fn supported_devices(&self) -> alloc::vec::Vec<&'static str> {
        vec!["simple-bus"]
    }

    fn instantiate(
        &self,
        _device: &crate::library::DeviceTreeNode,
        _parent: Option<&DeviceTreeNode>,
        _mapping_chain: &crate::library::MemoryMappingChain,
    ) -> Option<alloc::boxed::Box<dyn super::Driver>> {
        Some(alloc::boxed::Box::new(SimpleBusDriver))
    }
}

pub struct WwosMemoryDriver {
    addr: usize,
    size: usize,
}

impl Driver for WwosMemoryDriver {
    fn get_memory_mapping(&self) -> Option<crate::library::MemoryMapping> {
        return None;
    }

    fn as_memory(&self) -> Option<&dyn MemoryDriver> {
        Some(self)
    }
}

pub struct WwosMemoryDriverFactory;

impl super::DriverFactory for WwosMemoryDriverFactory {
    fn supported_devices(&self) -> alloc::vec::Vec<&'static str> {
        vec!["wwos,memory"]
    }

    fn instantiate(
        &self,
        device: &crate::library::DeviceTreeNode,
        parent: Option<&DeviceTreeNode>,
        mapping_chain: &crate::library::MemoryMappingChain,
    ) -> Option<alloc::boxed::Box<dyn super::Driver>> {
        let (addr, size) = device.get_address_size(parent?)?;
        let addr = mapping_chain.get_up(addr)? as *mut u32;

        Some(alloc::boxed::Box::new(WwosMemoryDriver {
            addr: addr as usize,
            size,
        }))
    }
}

impl MemoryDriver for WwosMemoryDriver {
    fn get_address(&self) -> usize {
        self.addr
    }

    fn get_size(&self) -> usize {
        self.size
    }
}
