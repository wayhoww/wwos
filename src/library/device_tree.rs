use alloc::{string::String, vec::Vec};

#[derive(Debug)]
struct DeviceTreeHeader {
    magic: u32,
    total_size: u32,
    off_dt_struct: u32,
    off_dt_strings: u32,
    off_mem_rsvmap: u32,
    version: u32,
    last_comp_version: u32,
    boot_cpuid_phys: u32,
    size_dt_strings: u32,
    size_dt_struct: u32,
}

impl DeviceTreeHeader {
    pub fn from_slice(slice: &[u8]) -> Self {
        Self {
            magic: u32::from_be_bytes([slice[0], slice[1], slice[2], slice[3]]),
            total_size: u32::from_be_bytes([slice[4], slice[5], slice[6], slice[7]]),
            off_dt_struct: u32::from_be_bytes([slice[8], slice[9], slice[10], slice[11]]),
            off_dt_strings: u32::from_be_bytes([slice[12], slice[13], slice[14], slice[15]]),
            off_mem_rsvmap: u32::from_be_bytes([slice[16], slice[17], slice[18], slice[19]]),
            version: u32::from_be_bytes([slice[20], slice[21], slice[22], slice[23]]),
            last_comp_version: u32::from_be_bytes([slice[24], slice[25], slice[26], slice[27]]),
            boot_cpuid_phys: u32::from_be_bytes([slice[28], slice[29], slice[30], slice[31]]),
            size_dt_strings: u32::from_be_bytes([slice[32], slice[33], slice[34], slice[35]]),
            size_dt_struct: u32::from_be_bytes([slice[36], slice[37], slice[38], slice[39]]),
        }
    }
}

fn load_string_from_null_terminated_slice(slice: &[u8]) -> String {
    let mut out = String::new();
    for &c in slice {
        if c == 0 {
            return out;
        }
        out.push(c as char);
    }
    panic!("Error: String not null terminated");
}

const FDT_BEGIN_NODE: u32 = 0x1;
const FDT_END_NODE: u32 = 0x2;
const FDT_PROP: u32 = 0x3;
const FDT_NOP: u32 = 0x4;
const FDT_END: u32 = 0x9;

pub struct DeviceTreePropertyValue(pub Vec<u8>);

impl DeviceTreePropertyValue {
    pub fn from_slice(slice: &[u8]) -> Self {
        Self(slice.to_vec())
    }
}

impl core::ops::Deref for DeviceTreePropertyValue {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl core::ops::DerefMut for DeviceTreePropertyValue {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl core::fmt::Debug for DeviceTreePropertyValue {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let str = core::str::from_utf8(&self.0);
        if let Ok(str) = str {
            core::fmt::Debug::fmt(str, f)
        } else {
            core::fmt::Debug::fmt(&self.0, f)
        }
    }
}

#[derive(Debug)]
pub struct DeviceTreeNode {
    pub name: String,
    pub properties: Vec<(String, DeviceTreePropertyValue)>,
    pub children: Vec<DeviceTreeNode>,
}

#[derive(Copy, Clone)]
pub enum DeviceTreeMatch {
    Exact,
    Contains,
}

fn contains_subarray(text: &[u8], pattern: &[u8]) -> bool {
    let mut p_text = 0;
    let mut p_pattern = 0;

    while p_text < text.len() {
        if text[p_text] == pattern[p_pattern] {
            p_pattern += 1;
            if p_pattern == pattern.len() {
                return true;
            }
        } else {
            p_pattern = 0;
        }

        p_text += 1;
    }

    false
}

impl DeviceTreeMatch {
    pub fn match_string(&self, text: &[u8], pattern: &[u8]) -> bool {
        match self {
            DeviceTreeMatch::Exact => text == pattern,
            DeviceTreeMatch::Contains => contains_subarray(text, pattern),
        }
    }
}

impl DeviceTreeNode {
    fn new() -> Self {
        Self {
            name: String::new(),
            properties: Vec::new(),
            children: Vec::new(),
        }
    }

    pub fn from_slice(structure_block: &[u8], strings_block: &[u8]) -> Self {
        let mut nodes: Vec<DeviceTreeNode> = Vec::new();

        let mut out: Option<DeviceTreeNode> = None;

        let mut p = 0;

        loop {
            let token_int = u32::from_be_bytes([
                structure_block[p],
                structure_block[p + 1],
                structure_block[p + 2],
                structure_block[p + 3],
            ]);
            p += 4;

            match token_int {
                FDT_BEGIN_NODE => {
                    if out.is_some() {
                        panic!("Error: FDT_BEGIN_NODE after FDT_END");
                    }

                    let mut new_node = DeviceTreeNode::new();

                    let name = load_string_from_null_terminated_slice(&structure_block[p..]);
                    p += name.len() + 1;
                    p = (p + 3) & !3;
                    new_node.name = name;

                    nodes.push(new_node);
                }
                FDT_END_NODE => {
                    let last = nodes.pop();
                    if let Some(new_last) = nodes.last_mut() {
                        new_last.children.push(last.unwrap());
                    } else {
                        out = Some(last.unwrap());
                    }
                }
                FDT_PROP => {
                    let len = u32::from_be_bytes([
                        structure_block[p],
                        structure_block[p + 1],
                        structure_block[p + 2],
                        structure_block[p + 3],
                    ]);
                    p += 4;

                    let nameoff = u32::from_be_bytes([
                        structure_block[p],
                        structure_block[p + 1],
                        structure_block[p + 2],
                        structure_block[p + 3],
                    ]);
                    p += 4;

                    let mut value = Vec::new();
                    for _ in 0..len {
                        value.push(structure_block[p]);
                        p += 1;
                    }

                    p = (p + 3) & !3;

                    let name =
                        load_string_from_null_terminated_slice(&strings_block[nameoff as usize..]);
                    nodes
                        .last_mut()
                        .unwrap()
                        .properties
                        .push((name, DeviceTreePropertyValue(value)));
                }
                FDT_NOP => {}
                FDT_END => break,
                _ => panic!("Unknown token: {}", token_int),
            }
        }

        out.unwrap()
    }

    fn find_nodes_by_property_recursively<'t>(
        self: &'t DeviceTreeNode,
        property: &str,
        query: &[u8],
        schema: DeviceTreeMatch,
        out: &mut Vec<&'t DeviceTreeNode>,
    ) {
        for (name, value) in &self.properties {
            if name != property {
                continue;
            }

            if schema.match_string(value, query) {
                out.push(self);
                break;
            }
        }

        for child in &self.children {
            child.find_nodes_by_property_recursively(property, query, schema, out);
        }
    }

    fn find_nodes_by_name_recursively<'t>(
        self: &'t DeviceTreeNode,
        name: &str,
        out: &mut Vec<&'t DeviceTreeNode>,
    ) {
        if self.name == name {
            out.push(self);
        }

        for child in &self.children {
            child.find_nodes_by_name_recursively(name, out);
        }
    }

    pub fn find_property_by_name(&self, name: &str) -> Option<&DeviceTreePropertyValue> {
        for (n, v) in &self.properties {
            if n == name {
                return Some(v);
            }
        }

        None
    }

    pub fn get_n_address_cells(&self) -> Option<u32> {
        let bytes = self.find_property_by_name("#address-cells")?;
        assert!(bytes.len() == 4);
        let out = u32::from_be_bytes(bytes[0..4].try_into().unwrap());
        assert!(out <= 2);
        Some(out)
    }

    pub fn get_n_size_cells(&self) -> Option<u32> {
        let bytes = self.find_property_by_name("#size-cells")?;
        assert!(bytes.len() == 4);
        let out = u32::from_be_bytes(bytes[0..4].try_into().unwrap());
        assert!(out <= 2);
        Some(out)
    }

    pub fn get_address_size(&self, parent: &DeviceTreeNode) -> Option<(usize, usize)> {
        let n_address_cells = parent.get_n_address_cells()? as usize;
        let n_size_cells = parent.get_n_size_cells()? as usize;
        let regs = self.find_property_by_name("reg")?;

        let address;
        let size;

        if n_address_cells == 1 {
            address = u32::from_be_bytes(regs[0..4].try_into().unwrap()) as usize;
        } else {
            address = u64::from_be_bytes(regs[0..8].try_into().unwrap()) as usize;
        }

        let size_begin = 4 * n_address_cells;

        if n_size_cells == 0 {
            size = 0usize;
        } else if n_size_cells == 1 {
            size =
                u32::from_be_bytes(regs[size_begin..size_begin + 4].try_into().unwrap()) as usize;
        } else {
            size =
                u64::from_be_bytes(regs[size_begin..size_begin + 8].try_into().unwrap()) as usize;
        }

        Some((address, size))
    }

    pub fn get_compatibles(&self) -> Vec<&str> {
        let mut out = Vec::new();
        if let Some(compatibles) = self.find_property_by_name("compatible") {
            let mut p = 0;
            while p < compatibles.len() {
                let mut end = p;
                while end < compatibles.len() && compatibles[end] != 0 {
                    end += 1;
                }
                out.push(core::str::from_utf8(&compatibles[p..end]).unwrap());
                p = end + 1;
            }
        }
        out
    }
}

#[derive(Debug)]
pub struct MemoryReservation {
    pub address: u64,
    pub size: u64,
}

impl MemoryReservation {
    fn from_slice(slice: &[u8]) -> Self {
        Self {
            address: u64::from_be_bytes([
                slice[0], slice[1], slice[2], slice[3], slice[4], slice[5], slice[6], slice[7],
            ]),
            size: u64::from_be_bytes([
                slice[8], slice[9], slice[10], slice[11], slice[12], slice[13], slice[14],
                slice[15],
            ]),
        }
    }
}

#[derive(Debug)]
pub struct DeviceTree {
    pub root: DeviceTreeNode,
    pub memory_reservations: Vec<MemoryReservation>,
}

impl DeviceTree {
    fn from_memory_internal(memory: *const u8) -> Self {
        let size_of_header = core::mem::size_of::<DeviceTreeHeader>();

        let header = DeviceTreeHeader::from_slice(unsafe {
            core::slice::from_raw_parts(memory, size_of_header)
        });
        if header.magic != 0xd00dfeed {
            panic!("Error: Invalid device tree magic");
        }

        let structure_block = unsafe {
            core::slice::from_raw_parts(
                memory.offset(header.off_dt_struct as isize),
                header.size_dt_struct as usize,
            )
        };
        let strings_block = unsafe {
            core::slice::from_raw_parts(
                memory.offset(header.off_dt_strings as isize),
                header.size_dt_strings as usize,
            )
        };

        let root = DeviceTreeNode::from_slice(structure_block, strings_block);

        let mut memory_reservations = Vec::new();
        let mut p = header.off_mem_rsvmap;
        while p < header.off_mem_rsvmap + 16 {
            let reservation = MemoryReservation::from_slice(unsafe {
                core::slice::from_raw_parts(memory.offset(p as isize), 16)
            });
            if reservation.address == 0 && reservation.size == 0 {
                break;
            }
            memory_reservations.push(reservation);
            p += 16;
        }

        Self {
            root,
            memory_reservations,
        }
    }

    pub unsafe fn from_memory(memory: *const u8) -> Self {
        Self::from_memory_internal(memory)
    }

    pub fn find_nodes_by_property(
        &self,
        property: &str,
        query: &[u8],
        schema: DeviceTreeMatch,
    ) -> Vec<&DeviceTreeNode> {
        let mut out = Vec::new();
        self.root
            .find_nodes_by_property_recursively(property, query, schema, &mut out);
        out
    }

    pub fn find_nodes_by_name(&self, name: &str) -> Vec<&DeviceTreeNode> {
        let mut out = Vec::new();
        self.root.find_nodes_by_name_recursively(name, &mut out);
        out
    }
}
