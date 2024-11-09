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
    list() = default;
    list(const list&) = delete;
    list& operator=(const list&) = delete;

    ~list() {
        T out;
        while(pop_front(out)); 
    }

    void push_back(const T& data) {
        auto new_node = new list_node<T>{nullptr, nullptr, data};
        if(tail == nullptr) {
            head = new_node;
            tail = new_node;
            head->next = tail;
            tail->prev = head;
        } else {
            tail->next = new_node;
            new_node->prev = tail;
            tail = new_node;
        }
        m_size++;
    }

    bool pop_front(T& out) {
        if(head == nullptr) {
            return false;
        }
        if(m_size == 1) {
            out = head->data;
            delete head;
            head = nullptr;
            tail = nullptr;
            return true;
        }
        out = head->data;
        auto next = head->next;
        delete head;
        head = next;
        head->prev = nullptr;
        m_size--;
    }

protected:
    // head and tail HOLDS data
    list_node<T>* head = nullptr;
    list_node<T>* tail = nullptr;
    size_t m_size = 0;
};

}


#endif