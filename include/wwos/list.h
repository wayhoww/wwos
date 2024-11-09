#ifndef _WWOS_LIST_H
#define _WWOS_LIST_H


#include "wwos/stdint.h"
namespace wwos {

template <typename T>
struct list_node {
    list_node* next;
    list_node* prev;
    T data;
};

template <typename T>
class list {
public:
    list() {
        head = new list_node<T>{nullptr, nullptr, T()};
        tail = new list_node<T>{nullptr, nullptr, T()};
        head->next = tail;
        tail->prev = head;
    }
    list(const list&) = delete;
    list& operator=(const list&) = delete;

    ~list() {
        T out;
        while(pop_front(out)); 
        delete head;
        delete tail;
    }

    void push_back(const T& data) {
        auto new_node = new list_node<T>{nullptr, nullptr, data};
        
        new_node->prev = tail->prev;
        new_node->next = tail;

        if(tail->prev) {
            tail->prev->next = new_node;
        }
        tail->prev = new_node;
        m_size++;
    }

    bool get_front(T& out) const {
        if(empty()) {
            return false;
        }
        out = head->next->data;
        return true;
    }

    bool pop_front(T& out) {
        if(empty()) {
            return false;
        }
        auto node = head->next;
        out = node->data;
        head->next = node->next;
        if(node->next) {
            node->next->prev = head;
        }
        delete node;
        m_size--;
        return true;
    }

    size_t size() const {
        return m_size;
    }

    bool empty() const {
        return m_size == 0;
    }

protected:
    // head and tail HOLDS NO data
    list_node<T>* head = nullptr;
    list_node<T>* tail = nullptr;
    size_t m_size = 0;
};

}


#endif