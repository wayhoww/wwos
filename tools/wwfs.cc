#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "wwos/wwfs.h"



using namespace wwos::wwfs;

std::vector<std::uint8_t> read_binary(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    file.seekg(0, std::ios::end);
    auto size = file.tellg();

    std::vector<std::uint8_t> buffer(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    return buffer;
}

void write_binary(const std::string& filename, const std::vector<std::uint8_t>& buffer) {
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

void initialize(wwos::wwfs::meta_t meta, const std::string& filename) {
    auto required_size = meta.required_size();
    std::vector<uint8_t> memory(required_size, 0);
    file_system_hardware_memory hw(memory.data(), meta.block_size, memory.size() / meta.block_size );
    basic_file_system fs(&hw);
    fs.format(meta);
    write_binary(filename, memory);
}


void dump_recursive(basic_file_system& fs, uint64_t inode_id, int depth) {
    auto children = fs.get_children(inode_id);

    for(auto& [name, id] : children) {
        for(int i = 0; i < depth; i++) {
            std::cout << "    ";
        }
        std::cout << name.c_str() << std::endl;
        if(fs.get_inode_type(id) == inode_type::directory) {
            dump_recursive(fs, id, depth + 1);
        }
    }
}

void dump(const std::string& filename) {
    auto buffer = read_binary(filename);
    meta_t meta;
    memcpy(&meta, buffer.data(), sizeof(meta_t));

    std::cout << "Block size: " << meta.block_size << std::endl;
    std::cout << "Block count: " << meta.block_count << std::endl;
    std::cout << "Inode count: " << meta.inode_count << std::endl;
    std::cout << "Size: " << meta.required_size() << std::endl;

    file_system_hardware_memory hw(buffer.data(), meta.block_size, meta.required_size() / meta.block_size);
    basic_file_system fs(&hw);
    fs.initialize();

    std::cout << "root" << std::endl;
    dump_recursive(fs, fs.get_root(), 1);
}

void add_file(const std::string& filename, const std::string& name, const std::string& path) {
    auto buffer = read_binary(filename);
    meta_t meta;
    memcpy(&meta, buffer.data(), sizeof(meta_t));
    file_system_hardware_memory hw(buffer.data(), meta.block_size, meta.required_size() / meta.block_size);
    basic_file_system fs(&hw);
    fs.initialize();

    auto file_to_add = read_binary(path);
    auto parent = fs.get_root();
    auto children = fs.get_children(parent);

    size_t path_index = 1;
    while(path_index < name.size()) {
        size_t next_index = name.find('/', path_index);
        if(next_index == std::string::npos) {
            next_index = name.size();
        }
        auto name_piece = name.substr(path_index, next_index - path_index);

        bool existing = false;
        for(auto& [child_name, child_id] : children) {
            if(std::string(child_name.c_str()) == name_piece) {
                parent = child_id;
                existing = true;
                break;
            }
        }

        if(next_index == name.size()) {
            if(existing) {
                fs.write_data(parent, 0, file_to_add.size(), file_to_add.data());
            } else {
                auto inode_id = fs.create(parent, name_piece.c_str(), inode_type::file);
                fs.initialize();
                fs.write_data(inode_id, 0, file_to_add.size(), file_to_add.data());
                fs.initialize();
            }
        } else {
            if(!existing) {
                parent = fs.create(parent, name_piece.c_str(), inode_type::directory);
            } 
        }

        path_index = next_index + 1;
    }
    write_binary(filename, buffer);
}


int main(int argc, const char* argv[]) {
    // wwfs initialize <binary file> <block_size> <block_count> <inode_count>
    // wwfs add <binary file> <name of new file> <path to new file> 
    // wwfs info <binary file>

    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [args...]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if(command == "initialize") {
        if(argc != 6) {
            std::cerr << "Usage: " << argv[0] << " initialize <binary file> <block_size> <block_count> <inode_count>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        std::uint64_t block_size = std::stoull(argv[3]);
        std::uint64_t block_count = std::stoull(argv[4]);
        std::uint64_t inode_count = std::stoull(argv[5]);

        meta_t meta = {
            .block_size = block_size,
            .block_count = block_count,
            .inode_count = inode_count,
        };

        initialize(meta, filename);
    } else if (command == "add") {
        if(argc != 5) {
            std::cerr << "Usage: " << argv[0] << " add <binary file> <name of new file> <path to new file>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        std::string name = argv[3];
        std::string path = argv[4];

        add_file(filename, name, path);
    } else if (command == "info") {
        if(argc != 3) {
            std::cerr << "Usage: " << argv[0] << " info <binary file>" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        dump(filename);
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }
    

    return 0;
}