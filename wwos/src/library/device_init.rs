use alloc::vec::Vec;
use alloc::vec;
use alloc::{boxed::Box, string::String};
use log::{info, warn};

use crate::drivers::{self, Driver, DriverFactory, Serial32Driver};

use super::{
    device_tree::{DeviceTree, DeviceTreeNode},
    memory_mapping::MemoryMappingChain,
};

fn overlaps(l1: &[&str], l2: &[&str]) -> bool {
    for s1 in l1.iter() {
        for s2 in l2.iter() {
            if s1 == s2 {
                return true;
            }
        }
    }
    false
}

pub struct DriverInstance {
    pub path: Vec<String>,
    pub driver: Box<dyn Driver>,
}

fn initialize_hardwares_recursive(
    node: &DeviceTreeNode,
    parent: Option<&DeviceTreeNode>,
    driver_factories: &Vec<Box<dyn DriverFactory>>,
    mapping_chain: &mut MemoryMappingChain,
    path: &mut Vec<String>,
    out: &mut Vec<DriverInstance>,
    depth: usize,
) {
    let node_compatibles = node.get_compatibles();
    path.push(node.name.clone());
    let path_string = path.iter().fold(String::new(), |acc, x| acc + "/" + x);

    let mut device_initialized = false;

    for driver in driver_factories.iter() {
        let supported_devices = driver.supported_devices();

        if !overlaps(&node_compatibles, &supported_devices) {
            continue;
        }

        let driver_instance = driver.instantiate(node, parent, mapping_chain);
        if driver_instance.is_none() {
            // TODO: logging
            info!("failed to initialize driver for device `{}`", path_string);
            continue;
        }

        device_initialized = true;

        let driver_instance = driver_instance.unwrap();
        info!("initialized driver for device `{}`", path_string);

        if let Some(mapping) = driver_instance.get_memory_mapping() {
            mapping_chain.mappings.push(mapping);
        }

        out.push(DriverInstance {
            path: path.clone(),
            driver: driver_instance,
        });

        break;
    }

    if !device_initialized {
        // TODO: logging
        warn!(
            "no driver found for device `{}`, compatible={:?}",
            path_string,
            node.get_compatibles()
        );
        path.pop();
        return;
    }

    let ranges = node.find_property_by_name("ranges");
    if depth > 0 && ranges.is_none() {
        path.pop();
        return;
    }

    for child in node.children.iter() {
        // TODO(ww): address mapping
        initialize_hardwares_recursive(
            child,
            Some(node),
            driver_factories,
            mapping_chain,
            path,
            out,
            depth + 1,
        );
    }

    path.pop();
}

fn initialize_hardwares_internal(
    tree: &DeviceTree,
    driver_factories: Vec<Box<dyn DriverFactory>>,
) -> Vec<DriverInstance> {
    let mut mapping_chain = MemoryMappingChain::new();
    let mut path = Vec::new();
    let mut driver_instances = Vec::new();

    initialize_hardwares_recursive(
        &tree.root,
        None,
        &driver_factories,
        &mut mapping_chain,
        &mut path,
        &mut driver_instances,
        0,
    );

    driver_instances
}

static mut DRIVER_INSTANCES: Option<Vec<DriverInstance>> = None;

pub fn initialize_hardwares(tree: &DeviceTree) {
    let driver_factories: Vec<Box<dyn DriverFactory>> = vec![
        Box::new(drivers::PL011DriverFactory),
        Box::new(drivers::DummyVirtDriverFactory),
        Box::new(drivers::Bcm2711DriverFactory),
        Box::new(drivers::SimpleBusDriverFactory),
        Box::new(drivers::MailboxDriverFactory),
    ];

    let instances = initialize_hardwares_internal(tree, driver_factories);
    unsafe {
        DRIVER_INSTANCES = Some(instances);
    }
}

pub fn get_serial32_drivers() -> Vec<&'static mut dyn Serial32Driver> {
    let driver_instances = unsafe { DRIVER_INSTANCES.as_mut().unwrap() };
    driver_instances
        .iter_mut()
        .flat_map(|inst| inst.driver.as_serial32_mut())
        .collect()
}