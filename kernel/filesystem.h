#ifndef _WWOS_KERNEL_FILESYSTEM_H
#define _WWOS_KERNEL_FILESYSTEM_H

#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/string_view.h"
namespace wwos::kernel {

void initialize_filesystem(void* addr, size_t size);

int64_t get_inode(string_view path);

size_t read_inode(void* buffer, int64_t id, size_t offset, size_t size);

size_t write_inode(void* buffer, int64_t id, size_t offset, size_t size);

size_t get_inode_size(int64_t id);

int64_t create_subdirectory(int64_t parent, string_view name);

int64_t create_file(int64_t parent, string_view name);

vector<pair<string, int64_t>> get_children(int64_t parent);

uint64_t get_flattened_children(int64_t parent, uint8_t* buffer, uint64_t size);

bool is_directory(int64_t id);

bool is_file(int64_t id);

}

#endif