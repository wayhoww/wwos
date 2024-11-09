#ifndef _WWOS_KERNEL_FILESYSTEM_H
#define _WWOS_KERNEL_FILESYSTEM_H

#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"
namespace wwos::kernel {


struct shared_file_node {
    int64_t inode;
    fd_type type;
    vector<int64_t> readers;
    vector<int64_t> writers;
};

// logical
shared_file_node* open_shared_file_node(uint64_t pid, string_view path, fd_mode mode);

bool close_shared_file_node(uint64_t pid, shared_file_node* sfn);

bool create_shared_file_node(string_view path, fd_type type);

void initialize_filesystem(void* addr, size_t size);

size_t read_shared_node(void* buffer, shared_file_node* node, size_t offset, size_t size);

size_t write_shared_node(void* buffer, shared_file_node* node, size_t offset, size_t size);

size_t get_shared_node_size(shared_file_node* node);

uint64_t get_flattened_children(shared_file_node* node, uint8_t* buffer, uint64_t size);

}

#endif