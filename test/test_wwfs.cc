#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cassert>

#include "wwos/wwfs.h"


int main() {
    using namespace wwos::wwfs;

    meta_t meta = {
        .block_size = 1024,
        .block_count = 1024 * 128,
        .inode_count = 1024,
    };

    auto required_size = meta.required_size();
    printf("Required size: %lx\n", required_size);

    assert(required_size % meta.block_size == 0);

    std::vector<uint8_t> memory(required_size);
    file_system_hardware_memory hw(memory.data(), meta.block_size, memory.size() / meta.block_size );
    basic_file_system fs(&hw);
    
    fs.format(meta);
    fs.initialize();

    auto f_usr = fs.create(fs.get_root(), "usr", inode_type::directory);
    auto f_bin = fs.create(fs.get_root(), "bin", inode_type::directory);
    auto f_etc = fs.create(fs.get_root(), "etc", inode_type::directory);
    auto f_home = fs.create(fs.get_root(), "home", inode_type::directory);
    
    auto f_home_weihaow = fs.create(f_home, "weihaow", inode_type::directory);
    auto f_etc_passwd = fs.create(f_etc, "passwd", inode_type::file);
    auto f_etc_group = fs.create(f_etc, "group", inode_type::file);
    auto f_etc_shadow = fs.create(f_etc, "shadow", inode_type::file);

    // write a small file to passwd
    std::string passwd = "root:x:0:0:root:/root:/bin/bash\n";
    fs.write_data(f_etc_passwd, 0, passwd.size(), passwd.data());
    
    std::vector<uint8_t> longfile_1(1024 * 6);
    for(size_t i = 0; i < longfile_1.size(); i++) {
        longfile_1[i] = rand() % 256;
    }
    fs.write_data(f_etc_group, 0, longfile_1.size(), longfile_1.data());

    std::vector<uint8_t> longfile_2(1024 * 20);
    for(size_t i = 0; i < longfile_2.size(); i++) {
        longfile_2[i] = rand() % 256;
    }
    fs.write_data(f_etc_shadow, 0, longfile_2.size(), longfile_2.data());

    // read back
    std::vector<uint8_t> buffer_0(passwd.size());
    fs.read_data(f_etc_passwd, 0, buffer_0.size(), buffer_0.data());
    assert(buffer_0 == std::vector<uint8_t>(passwd.begin(), passwd.end()));

    std::vector<uint8_t> buffer_1(longfile_1.size());
    fs.read_data(f_etc_group, 0, buffer_1.size(), buffer_1.data());
    assert(buffer_1 == longfile_1);

    std::vector<uint8_t> buffer_2(longfile_2.size());
    fs.read_data(f_etc_shadow, 0, buffer_2.size(), buffer_2.data());
    assert(buffer_2 == longfile_2);


    // write part of a file

    auto fs_home_weihaow_file = fs.create(f_home_weihaow, "file", inode_type::file);
    std::vector<uint8_t> longfile_3(545);
    std::vector<uint8_t> longfile_4(2412);
    std::vector<uint8_t> longfile_5(123);

    for(size_t i = 0; i < longfile_3.size(); i++) {
        longfile_3[i] = rand() % 256;
    }
    for(size_t i = 0; i < longfile_4.size(); i++) {
        longfile_4[i] = rand() % 256;
    }
    for(size_t i = 0; i < longfile_5.size(); i++) {
        longfile_5[i] = rand() % 256;
    }

    fs.write_data(fs_home_weihaow_file, 789, longfile_3.size(), longfile_3.data());
    fs.write_data(fs_home_weihaow_file, 789 + 545, longfile_4.size(), longfile_4.data());
    fs.write_data(fs_home_weihaow_file, 789 + 545 + 2412, longfile_5.size(), longfile_5.data());

    std::vector<uint8_t> buffer_3(longfile_3.size());
    fs.read_data(fs_home_weihaow_file, 789, buffer_3.size(), buffer_3.data());
    assert(buffer_3 == longfile_3);

    std::vector<uint8_t> buffer_4(longfile_4.size());
    fs.read_data(fs_home_weihaow_file, 789 + 545, buffer_4.size(), buffer_4.data());
    assert(buffer_4 == longfile_4);

    std::vector<uint8_t> buffer_5(longfile_5.size());
    fs.read_data(fs_home_weihaow_file, 789 + 545 + 2412, buffer_5.size(), buffer_5.data());
    assert(buffer_5 == longfile_5);

    std::vector<uint8_t> buffer_345(longfile_3.size() + longfile_4.size() + longfile_5.size());
    fs.read_data(fs_home_weihaow_file, 789, buffer_345.size(), buffer_345.data());

    std::vector<uint8_t> buffer_345_expected;
    buffer_345_expected.insert(buffer_345_expected.end(), buffer_3.begin(), buffer_3.end());
    buffer_345_expected.insert(buffer_345_expected.end(), buffer_4.begin(), buffer_4.end());
    buffer_345_expected.insert(buffer_345_expected.end(), buffer_5.begin(), buffer_5.end());

    assert(buffer_345 == buffer_345_expected);

    assert(fs.get_inode_size(fs_home_weihaow_file) == 789 + 545 + 2412 + 123);

    


    

    auto children = fs.get_children(fs.get_root());

    for(auto& [name, id] : children) {
        std::cout << name.c_str() << " " << id << std::endl;
    }

    assert(children[0].first == "usr");
    assert(children[0].second == f_usr);
    assert(children[1].first == "bin");
    assert(children[1].second == f_bin);
    assert(children[2].first == "etc");
    assert(children[2].second == f_etc);
    assert(children[3].first == "home");
    assert(children[3].second == f_home);

    children = fs.get_children(f_home);

    for(auto& [name, id] : children) {
        std::cout << name.c_str() << " " << id << std::endl;
    }

    assert(children[0].first == "weihaow");
    assert(children[0].second == f_home_weihaow);

    children = fs.get_children(f_etc);

    for(auto& [name, id] : children) {
        std::cout << name.c_str() << " " << id << std::endl;
    }

    assert(children[0].first == "passwd");
    assert(children[0].second == f_etc_passwd);
    assert(children[1].first == "group");
    assert(children[1].second == f_etc_group);


    std::cout << "Test case test_wwfs.cc passed." << std::endl;
    return 0;
}