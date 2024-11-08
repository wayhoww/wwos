#ifndef _WWOS_WWFS_H
#define _WWOS_WWFS_H


// ext2-like fs

// 1. superblock (meta data)

// 2. availability bitmap (each bit represent a block)
// 3. inodes, one of
//   2.1 directory
//   2.2 file
// 3. data blocks


#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/string.h"
#include "wwos/string_view.h"
#include "wwos/vector.h"

namespace wwos::wwfs {

enum class inode_type {
    directory,
    file,
};


struct inode {
    inode_type type;
    size_t size;
    size_t blocks_l0[10];
    size_t block_l1;

private:
    size_t _padding[3] __attribute__((unused));
};

static_assert(sizeof(inode) == 128, "Invalid inode size");


struct meta_t {
    size_t block_size;
    size_t block_count;

    size_t inode_count;

    size_t required_size() const {
        auto bid_meta = 0;
        auto bid_bitmap_inode = bid_meta + 1;
        auto bid_bitmap_data = bid_bitmap_inode + (inode_count + (8 * block_size - 1)) / (8 * block_size);
        auto bid_inodes = bid_bitmap_data + (block_count + (8 * block_size - 1)) / (8 * block_size);
        auto bid_data = bid_inodes + (inode_count + (block_size / sizeof(inode) - 1)) / (block_size / sizeof(inode));
        auto bid_upper = bid_data + block_count;
        return bid_upper * block_size;
    }
};


class file_system_hardware {
public:
    void read(size_t block, void* buffer);
    void write(size_t block, const void* buffer);
    size_t size() const;
    size_t get_block_size() const;
};

class file_system_hardware_memory: public file_system_hardware {
public:
    file_system_hardware_memory(void* memory, size_t block_size, size_t block_count);

    void read(size_t block, void* buffer);
    void write(size_t block, const void* buffer);
    size_t size() const { return block_count * block_size; }
    size_t get_block_size() const { return block_size; }
    
private:

    void* memory;
    size_t block_size;
    size_t block_count;
};

class file_system {
public:
    void initialize();

    void format(meta_t meta);

    int64_t get_root();

    int64_t create(int64_t parent, string_view name, inode_type type);

    vector<pair<string, int64_t>> get_children(int64_t inode_id);

    inode_type get_inode_type(int64_t id);

    size_t get_inode_size(int64_t id);

    size_t read_data(int64_t id, size_t offset, size_t size, void* buffer);

    size_t write_data(int64_t id, size_t offset, size_t size, const void* buffer);

    meta_t get_meta() const;
};

class basic_file_system: public file_system {
public:
    basic_file_system(file_system_hardware_memory* hw);

    void initialize();

    void format(meta_t meta);

    int64_t get_root();

    int64_t create(int64_t parent, string_view name, inode_type type);

    vector<pair<string, int64_t>> get_children(int64_t inode_id);

    inode_type get_inode_type(int64_t id);

    size_t get_inode_size(int64_t id);

    size_t read_data(int64_t id, size_t offset, size_t size, void* buffer);

    size_t write_data(int64_t id, size_t offset, size_t size, const void* buffer);

    meta_t get_meta() const  { return meta; }

protected:
    template <bool write=false>
    void rw_inode_data_unaligned(const inode& inode, size_t offset, size_t size, void* buffer);

    inode resize_inode(uint64_t inode_id, size_t new_size);

    uint64_t get_blockid_of_inode(inode inode, size_t index);

    void set_blockid_of_inode(inode& inode, size_t index, uint64_t blockid);

    inode get_inode(int64_t id);

    void set_inode(int64_t id, const inode& in);

    void read_block(uint64_t id, void* buffer);

    void write_block(uint64_t id, const void* buffer);

    int64_t allocate_inode();

    void deallocate_inode(int64_t id);

    uint64_t allocate_block();

    void deallocate_block(uint64_t id);
    
protected:
    size_t bid_meta = 0;
    size_t bid_bitmap_inode;
    size_t bid_bitmap_data;
    size_t bid_inodes;
    size_t bid_data;

    meta_t meta;
    file_system_hardware_memory* hw;

    bool initialized = false;
};


}
#endif