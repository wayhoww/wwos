use alloc::{boxed::Box, vec::Vec};

use crate::library::{DeviceTreeNode, MemoryMapping, MemoryMappingChain};

pub trait Driver {
    fn get_memory_mapping(&self) -> Option<MemoryMapping> {
        None
    }

    fn as_serial32(&self) -> Option<&dyn Serial32Driver> {
        None
    }
    fn as_serial32_mut(&mut self) -> Option<&mut dyn Serial32Driver> {
        None
    }
}

pub trait Serial32Driver: Driver {
    fn read(&self) -> u32;
    fn write(&mut self, data: u32);
}

impl core::fmt::Write for dyn Serial32Driver {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            self.write(c as u32);
        }
        Ok(())
    }
}

pub trait DriverFactory {
    fn supported_devices(&self) -> Vec<&'static str>;
    fn instantiate(
        &self,
        device: &DeviceTreeNode,
        parent: Option<&DeviceTreeNode>,
        mapping_chain: &MemoryMappingChain,
    ) -> Option<Box<dyn Driver>>;
}
