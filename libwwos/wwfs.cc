
// ext2-like fs

// 1. superblock (meta data)

// 2. availability bitmap (each bit represent a block)
// 3. inodes, one of
//   2.1 directory
//   2.2 file
// 3. data blocks


#include "wwos/wwfs.h"
#include "wwos/algorithm.h"
#include "wwos/assert.h"
#include "wwos/format.h"
#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/alloc.h"
#include "wwos/stdio.h"
#include "wwos/string.h"
#include "wwos/string_view.h"
#include "wwos/vector.h"

namespace wwos::wwfs {



file_system_hardware_memory::file_system_hardware_memory(void* memory, size_t block_size, size_t block_count) : memory(memory), block_size(block_size), block_count(block_count) {
    wwassert(memory != nullptr, "Invalid memory");
}

void file_system_hardware_memory::read(size_t block, void* buffer) {
    wwassert(block < block_count, "Invalid block");
    memcpy(buffer, reinterpret_cast<uint8_t*>(memory) + block * block_size, block_size);
}

void file_system_hardware_memory::write(size_t block, const void* buffer) {
    wwassert(block < block_count, "Invalid block");
    memcpy(reinterpret_cast<uint8_t*>(memory) + block * block_size, buffer, block_size);
}

basic_file_system::basic_file_system(file_system_hardware_memory* hw) : hw(hw) {
    wwassert(hw != nullptr, "Invalid hardware");
}

void basic_file_system::format(meta_t meta) {
    this->meta = meta;

    // write meta
    uint8_t* buffer = new uint8_t[meta.block_size];
    memset(buffer, 0, meta.block_size);
    memcpy(buffer, &meta, sizeof(meta));
    hw->write(bid_meta, buffer);

    initialize();


    // zero out bitmaps
    memset(buffer, 0, meta.block_size);
    for(size_t bid = bid_bitmap_inode; bid < bid_inodes; bid++) {
        hw->write(bid, buffer);
    }
    delete [] buffer;

    // create root directory
    uint64_t inode_root = allocate_inode();
    wwassert(inode_root == 0, "Invalid root inode");

    inode inode_root_raw = get_inode(inode_root);
    inode_root_raw.type = inode_type::DIRECTORY;
    inode_root_raw.size = 0;
    set_inode(inode_root, inode_root_raw);
}

void basic_file_system::initialize() {
    auto buffer __attribute__((unused)) = new uint8_t[hw->get_block_size()];

    hw->read(bid_meta, buffer);
    memcpy(&meta, buffer, sizeof(meta));
    delete [] buffer;

    wwassert(meta.block_size > 0 && meta.block_size % 1024 == 0, "Invalid block size");
    
    // format of the file system
    //                                                     count of blocks
    // meta                     (padding to block size)    1
    // bitmap of inode          (padding to block size)    (inode_count + (8 * block_size - 1)) / (8 * block_size)
    // bitmap of data blocks    (padding to block size)    (block_count + (8 * block_size - 1)) / (8 * block_size)
    // inodes                   (padding to block size)    inode_count + (block_size / sizeof(inode) - 1) / (block_size / sizeof(inode))
    // data blocks                                         block_count

    bid_meta = 0;
    bid_bitmap_inode = bid_meta + 1;
    bid_bitmap_data = bid_bitmap_inode + (meta.inode_count + (8 * meta.block_size - 1)) / (8 * meta.block_size);
    bid_inodes = bid_bitmap_data + (meta.block_count + (8 * meta.block_size - 1)) / (8 * meta.block_size);
    bid_data = bid_inodes + (meta.inode_count + (meta.block_size / sizeof(inode) - 1)) / (meta.block_size / sizeof(inode));   

    auto upperbound = (bid_data + meta.block_count) * meta.block_size;

    wwassert(hw->get_block_size() == meta.block_size, "Invalid block size");
    wwassert(hw->size() == upperbound, "Invalid hardware size");

    initialized = true;
}

int64_t basic_file_system::get_root() {
    return 0;
}

int64_t basic_file_system::create(int64_t parent, string_view name, inode_type type) {
    if(parent < 0) {
        return -1;
    }

    auto children = get_children(parent);
    // ensure no duplicate
    for(auto& [child_name, child_id]: children) {
        if(string_view(child_name) == name) {
            return -1;
        }
    }

    uint64_t inode_id_new = allocate_inode();
    wwassert(inode_id_new != ~0ull, "No available inode");

    inode inode_new = get_inode(inode_id_new);
    inode_new.type = type;
    inode_new.size = 0;
    set_inode(inode_id_new, inode_new);

    inode inode_parent = get_inode(parent);
    wwassert(inode_parent.type == inode_type::DIRECTORY, "Parent is not a directory");
    
    auto additional_size = align_up<size_t>(name.size() + 1, 8) + 8;
    auto new_parent_size = inode_parent.size + additional_size;
    inode_parent = resize_inode(parent, new_parent_size);

    // write new entry
    for(size_t i = 0; i < name.size(); i++) {
        wwassert(name[i] != 0, "Invalid name");
    }

    uint8_t* buffer = new uint8_t[additional_size];
    memset(buffer, 0, additional_size);
    memcpy(buffer, name.data(), name.size());
    *reinterpret_cast<uint64_t*>(buffer + align_up<size_t>(name.size() + 1, 8)) = inode_id_new;
    rw_inode_data_unaligned<true>(inode_parent, inode_parent.size - additional_size, additional_size, buffer);
    delete [] buffer;

    return inode_id_new;
}

vector<pair<string, int64_t>> basic_file_system::get_children(int64_t inode_id) {
    if(inode_id < 0) {
        return -1;
    }

    inode inode_raw = get_inode(inode_id);

    wwassert(inode_raw.type == inode_type::DIRECTORY, "Not a directory");

    if(inode_raw.size == 0) {
        return {};
    }

    // read all data from this inode
    uint8_t* buffer = new uint8_t[inode_raw.size];
    rw_inode_data_unaligned<false>(inode_raw, 0, inode_raw.size, buffer);

    vector<pair<string, int64_t>> out;
    size_t i = 0;
    while(i < inode_raw.size) {
        string_view name(reinterpret_cast<char*>(buffer + i));
        i += align_up<size_t>(name.size() + 1, 8);
        int64_t inode_id = *reinterpret_cast<int64_t*>(buffer + i);
        i += 8;
        out.push_back({string(name.data(), name.size()), inode_id});
    }

    delete [] buffer;
    return out;
}


template <bool write>
void basic_file_system::rw_inode_data_unaligned(const inode& inode, size_t offset, size_t size, void* buffer) {
    if(size == 0) {
        return;
    }

    auto begin_block_index = offset / meta.block_size;
    auto end_block_index = (offset + size - 1) / meta.block_size;

    for(size_t index = begin_block_index; index <= end_block_index; index++) {
        auto begin_addr_inode = max(offset, index * meta.block_size);
        auto end_addr_node = min(offset + size, (index + 1) * meta.block_size);

        auto begin_addr_buffer = begin_addr_inode - offset;

        if(begin_addr_inode == index * meta.block_size && end_addr_node == (index + 1) * meta.block_size) {
            if(write) {
                write_block(get_blockid_of_inode(inode, index), reinterpret_cast<uint8_t*>(buffer) + begin_addr_buffer);
            } else {
                read_block(get_blockid_of_inode(inode, index), reinterpret_cast<uint8_t*>(buffer) + begin_addr_buffer);
            }
        } else {
            auto* buffer_block = new uint8_t[meta.block_size];
            auto in_block_offset = begin_addr_inode - index * meta.block_size;
            if(write) {
                read_block(get_blockid_of_inode(inode, index), buffer_block);
                memcpy(buffer_block + in_block_offset, reinterpret_cast<uint8_t*>(buffer) + begin_addr_buffer, end_addr_node - begin_addr_inode);
                write_block(get_blockid_of_inode(inode, index), buffer_block);
            } else {
                read_block(get_blockid_of_inode(inode, index), buffer_block);
                memcpy(reinterpret_cast<uint8_t*>(buffer) + begin_addr_buffer, buffer_block + in_block_offset, end_addr_node - begin_addr_inode);
            }
            delete [] buffer_block;
        }
    }
}

inode basic_file_system::resize_inode(uint64_t inode_id, size_t new_size) {
    inode inode_new = get_inode(inode_id);

    auto inode_old_size = inode_new.size;
    auto inode_new_size = new_size;

    if(inode_old_size == inode_new_size) {
        return inode_new;
    }

    inode_new.size = new_size;

    auto inode_old_block_count = (inode_old_size + meta.block_size - 1) / meta.block_size;
    auto inode_new_block_count = (inode_new_size + meta.block_size - 1) / meta.block_size;

    if(inode_old_block_count == inode_new_block_count) {
        set_inode(inode_id, inode_new);
        return inode_new;
    }

    if(inode_old_block_count < inode_new_block_count) {
        // expand

        if(inode_old_block_count < 10 && inode_new_block_count >= 10) {
            auto block_id = allocate_block();
            wwassert(block_id >= 0, "No available block");
            inode_new.block_l1 = block_id;
        }

        for(size_t i = inode_old_block_count; i < inode_new_block_count; i++) {
            auto block_id = allocate_block();
            wwassert(block_id >= 0, "No available block");
            set_blockid_of_inode(inode_new, i, block_id);
        }
    } else {
        // shrink
        for(size_t i = inode_new_block_count; i < inode_old_block_count; i++) {
            deallocate_block(get_blockid_of_inode(inode_new, i));
        }
    }

    set_inode(inode_id, inode_new);
    return inode_new;
}

uint64_t basic_file_system::get_blockid_of_inode(inode inode, size_t index) {
    if(index < 10) {
        return inode.blocks_l0[index];
    } else if(index < 10 + meta.block_size / sizeof(uint64_t)) {
        auto* buffer = new uint8_t[meta.block_size];
        this->read_block(inode.block_l1, buffer);
        auto out = reinterpret_cast<uint64_t*>(buffer)[index - 10];
        delete [] buffer;
        return out;
    } else {
        wwassert(false, "Invalid index");
    }
}

void basic_file_system::set_blockid_of_inode(inode& inode, size_t index, uint64_t blockid) {
    if(index < 10) {
        inode.blocks_l0[index] = blockid;
    } else if(index < 10 + meta.block_size / sizeof(uint64_t)) {
        auto* buffer = new uint8_t[meta.block_size];
        this->read_block(inode.block_l1, buffer);
        reinterpret_cast<uint64_t*>(buffer)[index - 10] = blockid;
        this->write_block(inode.block_l1, buffer);
        delete [] buffer;
    } else {
        wwassert(false, "Invalid index");
    }
}

// receive inode id, return inode
inode basic_file_system::get_inode(int64_t id) {
    size_t bid = bid_inodes + id / (meta.block_size / sizeof(inode));
    size_t offset = (id % (meta.block_size / sizeof(inode))) * sizeof(inode);
    uint8_t* buffer = new uint8_t[meta.block_size];
    hw->read(bid, buffer);
    inode out = *reinterpret_cast<inode*>(buffer + offset);
    delete [] buffer;
    return out;
}

void basic_file_system::set_inode(int64_t id, const inode& in) {
    size_t bid = bid_inodes + id / (meta.block_size / sizeof(inode));
    size_t offset = (id % (meta.block_size / sizeof(inode))) * sizeof(inode);
    uint8_t* buffer = new uint8_t[meta.block_size];
    hw->read(bid, buffer);
    *reinterpret_cast<inode*>(buffer + offset) = in;
    hw->write(bid, buffer);
    delete [] buffer;
}

void basic_file_system::read_block(uint64_t id, void* buffer) {
    hw->read(bid_data + id, buffer);
}

void basic_file_system::write_block(uint64_t id, const void* buffer) {
    hw->write(bid_data + id, buffer);
}

// return inode id
int64_t basic_file_system::allocate_inode() {
    // find a free inode
    uint8_t* buffer = new uint8_t[meta.block_size];
    for(size_t bid = bid_bitmap_inode; bid < bid_bitmap_data; bid++) {
        hw->read(bid, buffer);
        for(size_t i = 0; i < meta.block_size; i++) {
            for(size_t j = 0; j < 8; j++) {
                if((buffer[i] & (1 << j)) == 0) {
                    buffer[i] |= (1 << j);
                    hw->write(bid, buffer);
                    delete [] buffer;
                    return (bid - bid_bitmap_inode) * meta.block_size * 8 + i * 8 + j;
                }
            }
        }
    }
    delete [] buffer;
    return -1;
}

void basic_file_system::deallocate_inode(int64_t id) {
    size_t bid = bid_bitmap_inode + id / (meta.block_size * 8);
    size_t offset = id % (meta.block_size * 8);
    uint8_t* buffer = new uint8_t[meta.block_size];
    hw->read(bid, buffer);
    buffer[offset / 8] &= ~(1 << (offset % 8));
    hw->write(bid, buffer);
    delete [] buffer;
}

uint64_t basic_file_system::allocate_block() {
    // find a free block
    uint8_t* buffer = new uint8_t[meta.block_size];
    for(size_t bid = bid_bitmap_data; bid < bid_inodes; bid++) {
        hw->read(bid, buffer);
        for(size_t i = 0; i < meta.block_size; i++) {
            for(size_t j = 0; j < 8; j++) {
                if((buffer[i] & (1 << j)) == 0) {
                    buffer[i] |= (1 << j);
                    hw->write(bid, buffer);
                    delete [] buffer;
                    return (bid - bid_bitmap_data) * meta.block_size * 8 + i * 8 + j;
                }
            }
        }
    }
    delete [] buffer;
    return -1;
}

void basic_file_system::deallocate_block(uint64_t id) {
    size_t bid = bid_bitmap_data + id / (meta.block_size * 8);
    size_t offset = id % (meta.block_size * 8);
    uint8_t* buffer = new uint8_t[meta.block_size];
    hw->read(bid, buffer);
    buffer[offset / 8] &= ~(1 << (offset % 8));
    hw->write(bid, buffer);
    delete [] buffer;
}


inode_type basic_file_system::get_inode_type(int64_t id) {
    wwassert(id >= 0, "Invalid inode id");
    return get_inode(id).type;
}

size_t basic_file_system::read_data(int64_t id, size_t offset, size_t size, void* buffer) {
    if(size == 0) {
        return 0;
    }

    if(id < 0) {
        return 0;
    }

    auto inode_raw = get_inode(id);
    wwassert(inode_raw.type == inode_type::FILE, "Not a file");
    wwassert(offset + size <= inode_raw.size, "Invalid size");
    if(offset + size > inode_raw.size) {
        size = inode_raw.size - offset;
    }
    if(size == 0) {
        return 0;
    }
    rw_inode_data_unaligned<false>(inode_raw, offset, size, buffer);
    return size;
}

size_t basic_file_system::write_data(int64_t id, size_t offset, size_t size, const void* buffer) {
    if(id < 0) {
        return 0;
    }

    if(size == 0) {
        return 0;
    }

    auto inode_raw = get_inode(id);

    wwassert(inode_raw.type == inode_type::FILE, "Not a file");

    if(offset + size > inode_raw.size) {
        inode_raw = resize_inode(id, offset + size);
    }
    if(size == 0) {
        return 0;
    }
    rw_inode_data_unaligned<true>(inode_raw, offset, size, const_cast<void*>(buffer));
    return size;
}

size_t basic_file_system::get_inode_size(int64_t id) {
    wwassert(id >= 0, "Invalid inode id");
    return get_inode(id).size;
}



}