#include "wwos/algorithm.h"
#include "wwos/alloc.h"
#include "wwos/assert.h"
#include "wwos/format.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/wwfs.h"

namespace wwos::kernel {
    wwos::wwfs::file_system_hardware_memory* fs_hw_memory = nullptr;
    wwos::wwfs::basic_file_system* fs = nullptr;

    void initialize_filesystem(void* addr, size_t size) {
        wwassert(size >= sizeof(wwos::wwfs::meta_t), "Invalid size");

        wwos::wwfs::meta_t meta;
        memcpy(&meta, addr, sizeof(wwos::wwfs::meta_t));
        fs_hw_memory = new wwos::wwfs::file_system_hardware_memory(addr, meta.block_size, meta.required_size() / meta.block_size);
        
        fs = new wwos::wwfs::basic_file_system(fs_hw_memory);
        fs->initialize();
    }

    int64_t get_inode(string_view path) {
        if(path == "/") {
            return fs->get_root();
        }
        if(path.size() == 0) {
            return -1;
        }

        if(path[0] != '/') {
            return -1;
        }

        if(path[path.size() - 1] == '/' && path.size() > 1) {
            path = path.substr(0, path.size() - 1);
        }

        auto parent = fs->get_root();
        auto children = fs->get_children(parent);

        size_t path_index = 1;
        while(path_index < path.size()) {
            size_t next_index = path.find('/', path_index);
            if(next_index == string_view::npos) {
                next_index = path.size();
            }
            auto name_piece = path.substr(path_index, next_index - path_index);

            bool found = false;
            for(auto& [child_name, child_id] : children) {
                if(string_view(child_name) == name_piece) {
                    parent = child_id;
                    found = true;
                    break;
                }
            }

            if(!found) {
                return -1;
            }

            if(next_index == path.size()) {
                return parent;
            }

            auto nodetype = fs->get_inode_type(parent);
            if(nodetype != wwos::wwfs::inode_type::directory) {
                return -1;
            }

            children = fs->get_children(parent);
            path_index = next_index + 1;
        }

        return -1;
    }

    size_t read_inode(void* buffer, int64_t id, size_t offset, size_t size) {
        wwassert(id >= 0, "Invalid inode id");
        return fs->read_data(id, offset, size, buffer);
    }

    size_t write_inode(void* buffer, int64_t id, size_t offset, size_t size) {
        wwassert(id >= 0, "Invalid inode id");
        return fs->write_data(id, offset, size, buffer);
    }

    size_t get_inode_size(int64_t id) {
        wwassert(id >= 0, "Invalid inode id");
        return fs->get_inode_size(id);
    }

    bool is_directory(int64_t id) {
        wwassert(id >= 0, "Invalid inode id");
        return fs->get_inode_type(id) == wwos::wwfs::inode_type::directory;
    }

    bool is_file(int64_t id) {
        wwassert(id >= 0, "Invalid inode id");
        return fs->get_inode_type(id) == wwos::wwfs::inode_type::file;
    }

    int64_t create_subdirectory(int64_t parent, string_view name) {
        wwassert(parent >= 0, "Invalid parent id");
        return fs->create(parent, name, wwfs::inode_type::directory);
    }

    int64_t create_file(int64_t parent, string_view name) {
        wwassert(parent >= 0, "Invalid parent id");
        return fs->create(parent, name, wwfs::inode_type::file);
    }

    vector<pair<string, int64_t>> get_children(int64_t parent) {
        wwassert(parent >= 0, "Invalid parent id");
        return fs->get_children(parent);
    }
    
    uint64_t get_flattened_children(int64_t parent, uint8_t* buffer, uint64_t size) {
        wwassert(parent >= 0, "Invalid parent id");
        // return extra bytes required. 0 means succ

        auto children = fs->get_children(parent);
        uint64_t offset = 0;
        for(auto& [name, id]: children) {
            auto begin_name = offset;
            offset += name.size() + 1;
            auto end_name = offset;
            offset = align_up<size_t>(offset, 8);
            auto begin_id = offset;
            offset += 8;
            auto end_id = offset;

            if(begin_name < size) {
                memcpy(buffer + begin_name, name.data(), min<size_t>(end_name - begin_name, size - begin_name));
            }

            for(size_t i = end_name; i < begin_id && i < size; i++) {
                buffer[i] = 0;
            }

            if(begin_id + 8 <= size) {
                memcpy(buffer + begin_id, &id, min<size_t>(end_id - begin_id, size - begin_id));
            }
        }

        if(offset > size) {
            return offset - size;
        } else {
            if(offset < size) {
                memset(buffer + offset, 0, size - offset);
            }
            return 0;
        }
    }
    


}