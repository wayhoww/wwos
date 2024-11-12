#ifndef _WWOS_QUEUE_H
#define _WWOS_QUEUE_H

#include "wwos/stdint.h"
#include "wwos/vector.h"
namespace wwos {

template <typename T>
class cycle_queue {
public:
    cycle_queue(size_t capacity): m_data(capacity), m_head(0), m_tail(0), m_size(0) {}
    
    bool push(const T& data) {
        if(m_size == m_data.size()) {
            return false;
        }
        m_data[m_tail] = data;
        m_tail = (m_tail + 1) % m_data.size();
        m_size++;
        return true;
    }

    bool push_front(const T& data) {
        if(m_size == m_data.size()) {
            return false;
        }
        m_head = (m_head - 1 + m_data.size()) % m_data.size();
        m_data[m_head] = data;
        m_size++;
        return true;
    }

    bool pop(T& out) {
        if(m_size == 0) {
            return false;
        }
        out = m_data[m_head];
        m_head = (m_head + 1) % m_data.size();
        m_size--;
        return true;
    }

    bool get_front(T& out) const {
        if(m_size == 0) {
            return false;
        }
        out = m_data[m_head];
        return true;
    }

    bool pop_back(T& out) {
        if(m_size == 0) {
            return false;
        }
        out = m_data[(m_tail - 1 + m_data.size()) % m_data.size()];
        m_tail = (m_tail - 1 + m_data.size()) % m_data.size();
        m_size--;
        return true;
    }

    void clear() {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    size_t size() const {
        return m_size;
    }

    bool full() const {
        return m_size == m_data.size();
    }

    vector<T> items() const {
        vector<T> out;
        for(size_t i = 0; i < m_size; i++) {
            out.push_back(m_data[(m_head + i) % m_data.size()]);
        }
        return out;
    }

protected:
    vector<T> m_data;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_size = 0;
};


};

#endif