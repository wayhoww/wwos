#ifndef _WWOS_MAP_H
#define _WWOS_MAP_H

#include "wwos/assert.h"
#include "wwos/avl.h"
#include "wwos/format.h"
#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/vector.h"
namespace wwos {


template <typename K, typename V>
struct map_kv {
    K key;
    V value;

    bool operator<(const map_kv& other) const {
        return key < other.key;
    }
};

template <typename K, typename V>
class map {
public:
    map() = default;
    ~map() = default;

    map(const map& other) {
        auto items = other.items();
        for(auto& item: items) {
            insert(item.first, item.second);
        }
    }

    map& operator=(const map& other) {
        clear();
        auto items = other.items();
        for(auto& item: items) {
            insert(item.first, item.second);
        }
        return *this;
    }

    void insert(const K& key, const V& value) {
        wwassert(!contains(key), "key already exists");
        m_tree.insert({key, value});
    }
    void remove(const K& key) {
        wwassert(contains(key), "key not found");
        auto node = m_tree.find({key, V()});
        m_tree.remove(node);
    }
    V& get(const K& key) {
        wwassert(contains(key), "key not found");
        return m_tree.find({key, V()})->data.value;
    }
    vector<pair<K, V>> items() const {
        auto items = m_tree.items();
        vector<pair<K, V>> result;
        for(auto& item: items) {
            result.push_back({item.key, item.value});
        }
        return result;
    }
    bool contains(const K& key) {
        auto found = m_tree.find_exact({key, V()});
        if(found == nullptr) {
            return false;
        }
        return true;
    }
    size_t size() {
        return m_tree.size();
    }
    bool empty() {
        return m_tree.empty();
    }
    void clear() {
        m_tree.clear();
    }
protected:
    avl_tree<map_kv<K, V>> m_tree;
};

}

#endif