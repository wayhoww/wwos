#include "filesystem.h"
#include "process.h"
#include "wwos/algorithm.h"
#include "wwos/alloc.h"
#include "wwos/assert.h"
#include "wwos/avl.h"
#include "wwos/format.h"
#include "wwos/list.h"
#include "wwos/map.h"
#include "wwos/queue.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"
#include "wwos/vector.h"
#include "wwos/wwfs.h"

namespace wwos::kernel {
    constexpr size_t FIFO_SIZE = 1 << 20; // 1 MB buffer

    struct fifo_file {
        fifo_file(): fifo(cycle_queue<uint8_t>(FIFO_SIZE)) {}
        cycle_queue<uint8_t> fifo;
    };


    wwos::wwfs::file_system_hardware_memory* fs_hw_memory = nullptr;
    wwos::wwfs::basic_file_system* fs = nullptr;

    wwos::map<shared_file_node*, uint64_t>* p_snode_inode;
    wwos::map<uint64_t, shared_file_node*>* p_inode_snode;

    wwos::map<shared_file_node*, fifo_file>* p_fifo;



    void initialize_filesystem(void* addr, size_t size) {
        wwassert(size >= sizeof(wwos::wwfs::meta_t), "Invalid size");

        wwos::wwfs::meta_t meta;
        memcpy(&meta, addr, sizeof(wwos::wwfs::meta_t));
        fs_hw_memory = new wwos::wwfs::file_system_hardware_memory(addr, meta.block_size, meta.required_size() / meta.block_size);
        
        fs = new wwos::wwfs::basic_file_system(fs_hw_memory);
        fs->initialize();

        p_snode_inode = new map<shared_file_node*, uint64_t>();
        p_inode_snode = new map<uint64_t, shared_file_node*>();
        p_fifo = new map<shared_file_node*, fifo_file>();
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
            if(nodetype != wwos::wwfs::inode_type::DIRECTORY) {
                return -1;
            }

            children = fs->get_children(parent);
            path_index = next_index + 1;
        }

        return -1;
    }

    size_t read_shared_node(void* buffer, shared_file_node* node, size_t offset, size_t size) {
        wwassert(node, "Invalid inode id");
        if(node->type == fd_type::FIFO) {
            auto& fifo = p_fifo->get(node);
            auto read_size = min(fifo.fifo.size(), size);
            if(read_size != 0) {
                wwfmtlog("reading {} bytes from fifo", read_size);
            }
            for(size_t i = 0; i < read_size; i++) {
                wwassert(fifo.fifo.pop(((uint8_t*)buffer)[i]), "FIFO pop failed");
            }
            return read_size;
        }

        if(node->type == fd_type::DIRECTORY) {
            return 0;
        }

        return fs->read_data(node->inode, offset, size, buffer);
    }

    size_t write_shared_node(void* buffer, shared_file_node* node, size_t offset, size_t size) {
        wwassert(node, "Invalid inode id");
        if(node->type == fd_type::FIFO) {
            auto& fifo = p_fifo->get(node);
            auto write_size = min(FIFO_SIZE - fifo.fifo.size(), size);
            for(size_t i = 0; i < write_size; i++) {
                wwassert(fifo.fifo.push(((uint8_t*)buffer)[i]), "FIFO push failed");
            }
            return write_size;
        } 

        return fs->write_data(node->inode, offset, size, buffer);
    }

    size_t get_shared_node_size(shared_file_node* node) {
        wwassert(node, "Invalid inode id");
        if(node->type == fd_type::FIFO) {
            return p_fifo->get(node).fifo.size();
        }
        return fs->get_inode_size(node->inode);
    }

    vector<pair<string, int64_t>> get_children(int64_t parent) {
        wwassert(parent >= 0, "Invalid parent id");
        return fs->get_children(parent);
    }
    
    uint64_t get_flattened_children(shared_file_node* node, uint8_t* buffer, uint64_t size) {
        wwassert(node, "Invalid inode id");
        auto parent = node->inode;
        // return extra bytes required. 0 means succ
        auto children = fs->get_children(parent);
        uint64_t required_size = 0;
        for(auto& [name, id]: children) {
            required_size += name.size() + 1;
        }
        if(size < required_size) {
            return required_size;
        }
        size_t i = 0;
        for(auto& [name, id]: children) {
            memcpy(buffer + i, name.data(), name.size());
            i += name.size();
            buffer[i++] = '\0';
        }
        for(; i < size; i++) {
            buffer[i] = '\0';
        }
        return 0;
    }

    void init_fifo(shared_file_node* sfn) {
        wwlog("initing fifo");
        fifo_file ff;
        p_fifo->insert(sfn, ff);
    }
    
    shared_file_node* open_shared_file_node(uint64_t pid, string_view path, fd_mode mode) {
        wwfmtlog("trying to open {} with mode {}. by {}", path, static_cast<int>(mode), pid);

        auto inode = get_inode(path);
        if(inode < 0) {
            return nullptr;
        }

        auto node_type = fs->get_inode_type(inode);
        if(node_type == wwfs::inode_type::DIRECTORY && mode == fd_mode::WRITEONLY) {
            return nullptr;
        }

        if(!p_inode_snode->contains(inode)) {
            // create one!
            auto sfn = new shared_file_node();
            sfn->inode = inode;
            
            if(node_type == wwfs::inode_type::DIRECTORY) {
                sfn->type = fd_type::DIRECTORY;
            } else if(node_type == wwfs::inode_type::FILE) {
                sfn->type = fd_type::FILE;
            } else {
                sfn->type = fd_type::FIFO;
                init_fifo(sfn);
            } 

            if(mode == fd_mode::READONLY) {
                sfn->readers.push_back(pid);
            } else {
                sfn->writers.push_back(pid);
                if(sfn->type == fd_type::FILE) {
                    fs->resize_inode(sfn->inode, 0);
                }
            }
            p_inode_snode->insert(inode, sfn);
            p_snode_inode->insert(sfn, inode);
            return sfn;
        } else {
            auto sfn = p_inode_snode->get(inode);
            if(mode == fd_mode::READONLY) {
                sfn->readers.push_back(pid);
            } else {
                sfn->writers.push_back(pid);
                if(sfn->type == fd_type::FILE) {
                    fs->resize_inode(sfn->inode, 0);
                }
            }
            return sfn;
        }
    }

    bool close_shared_file_node(uint64_t pid, shared_file_node* sfn) {
        wwassert(sfn, "Invalid inode id");

        bool action = false;

        auto it = sfn->readers.find(pid);
        if(it != sfn->readers.end()) {
            sfn->readers.erase(it);
            action = true;
        } 

        it = sfn->writers.find(pid);
        if(it != sfn->writers.end()) {
            sfn->writers.erase(it);
            action = true;
        }

        if(sfn->writers.size() == 0 && sfn->readers.size() == 0) {
            
            if(sfn->type == fd_type::FIFO) {
                auto fifo = p_fifo->get(sfn);
                if(fifo.fifo.size() > 0) {
                    wwfmtlog("fifo has {} bytes left", fifo.fifo.size());
                    return false;
                }
            }

            p_inode_snode->remove(p_snode_inode->get(sfn));
            p_snode_inode->remove(sfn);
            if(sfn->type == fd_type::FIFO) {
                auto fifo = p_fifo->get(sfn);
                p_fifo->remove(sfn);
            }
            delete sfn;
        }

        return action;
    }

    pair<string_view, string_view> get_parent_path(string_view path) {
        if(path == "/" || path.size() == 0) {
            return {"", ""};
        }

        if(path[path.size() - 1] == '/') {
            path = path.substr(0, path.size() - 1);
        }

        size_t last_slash = path.find_last_of('/');
        if(last_slash == string_view::npos) {
            return {"", ""};
        }

        auto parent_path = path.substr(0, last_slash);
        if(parent_path.size() == 0) {
            parent_path = "/";
        }

        return {parent_path, path.substr(last_slash + 1, path.size() - last_slash - 1)};
    }

    bool create_shared_file_node(string_view path, fd_type type) {
        if(path.size() == 0 || path[0] != '/') return false;

        auto [parent_path, name] = get_parent_path(path);
        if(parent_path.size() == 0) {
            wwfmtlog("invalid path {}", path);
            return false;
        }

        auto parent_inode = get_inode(parent_path);
        if(parent_inode < 0) {
            wwfmtlog("failed to get parent inode of {}", parent_path);
            return false;
        }

        auto parent_type = fs->get_inode_type(parent_inode);
        if(parent_type != wwfs::inode_type::DIRECTORY) {
            wwlog("parent is not a directory");
            return false;
        }

        wwos::wwfs::inode_type itype;
        if(type == fd_type::DIRECTORY) {
            itype = wwfs::inode_type::DIRECTORY;
        } else if(type == fd_type::FILE) {
            itype = wwfs::inode_type::FILE;
        } else {
            itype = wwfs::inode_type::FIFO;
        }

        auto inode = fs->create(parent_inode, name, itype);
        if(inode < 0) {
            wwlog("failed to create inode");
            return false;
        }

        return true;
    }


}