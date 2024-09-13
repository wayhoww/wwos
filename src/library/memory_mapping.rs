use alloc::vec::Vec;


pub struct MemoryMappingItem {
    pub begin_up: usize,
    pub begin_down: usize,
    pub size: usize,
}

pub struct MemoryMapping {
    pub items: Vec<MemoryMappingItem>
}

impl MemoryMapping {
    pub fn new() -> MemoryMapping {
        MemoryMapping {
            items: Vec::new()
        }
    }

    pub fn add(&mut self, begin_up: usize, begin_down: usize, size: usize) {
        self.items.push(MemoryMappingItem {
            begin_up,
            begin_down,
            size
        });
    }

    pub fn get_up(&self, down: usize) -> Option<usize> {
        for item in self.items.iter() {
            if down >= item.begin_down && down < item.begin_down + item.size {
                return Some(item.begin_up + down - item.begin_down);
            }
        }
        None
    }

    pub fn get_down(&self, up: usize) -> Option<usize> {
        for item in self.items.iter() {
            if up >= item.begin_up && up < item.begin_up + item.size {
                return Some(item.begin_down + up - item.begin_up);
            }
        }
        None
    }
}


pub struct MemoryMappingChain {
    pub mappings: Vec<MemoryMapping>
}


impl MemoryMappingChain {
    pub fn new() -> MemoryMappingChain {
        MemoryMappingChain {
            mappings: Vec::new()
        }
    }

    pub fn add(&mut self, mapping: MemoryMapping) {
        self.mappings.push(mapping);
    }

    pub fn pop(&mut self) {
        self.mappings.pop();
    }

    pub fn get_up(&self, down: usize) -> Option<usize> {
        let mut up = down;
        for mapping in self.mappings.iter().rev() {
            if let Some(u) = mapping.get_up(up) {
                up = u;
            } else {
                return None;
            }
        }
        Some(up)
    }

    pub fn get_down(&self, up: usize) -> Option<usize> {
        let mut down = up;
        for mapping in self.mappings.iter() {
            if let Some(d) = mapping.get_down(down) {
                down = d;
            } else {
                return None;
            }
        }
        Some(down)
    }
}
