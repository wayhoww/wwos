#ifndef _WWOS_VECTOR_H
#define _WWOS_VECTOR_H

#include "wwos/assert.h"
#include "wwos/stdint.h"
#include "wwos/type_traits.h"

namespace wwos {

template<typename T>
class vector {
public:
    class iterator {
    public:
        iterator(T* ptr) : m_ptr(ptr) {}

        T& operator*() {
            return *m_ptr;
        }

        iterator& operator++() {
            m_ptr++;
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            m_ptr++;
            return temp;
        }

        iterator& operator--() {
            m_ptr--;
            return *this;
        }

        iterator operator--(int) {
            iterator temp = *this;
            m_ptr--;
            return temp;
        }

        iterator operator+=(size_t n) {
            m_ptr += n;
            return *this;
        }

        iterator operator-=(size_t n) {
            m_ptr -= n;
            return *this;
        }

        iterator operator+(size_t n) const {
            return iterator(m_ptr + n);
        }

        iterator operator-(size_t n) const {
            return iterator(m_ptr - n);
        }

        bool operator==(const iterator& other) const {
            return m_ptr == other.m_ptr;
        }

        bool operator!=(const iterator& other) const {
            return m_ptr != other.m_ptr;
        }

        friend class vector;

    protected:
        T* m_ptr;
    };

public:
    vector() : m_capacity(16), m_size(0), m_data(new T[16]) {}

    vector(size_t size) : m_capacity(size), m_size(size), m_data(new T[size]) {}

    vector(const vector& other) : m_capacity(other.m_capacity), m_size(other.m_size), m_data(new T[other.m_capacity]) {
        for(size_t i = 0; i < m_size; i++) {
            m_data[i] = other.m_data[i];
        }
    }

    vector(vector&& other) : m_capacity(other.m_capacity), m_size(other.m_size), m_data(other.m_data) {
        other.m_data = nullptr;
    }

    vector& operator=(const vector& other) {
        if(this == &other) {
            return *this;
        }
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        delete[] m_data;
        m_data = new T[other.m_capacity];
        for(size_t i = 0; i < m_size; i++) {
            m_data[i] = other.m_data[i];
        }
        return *this;
    }

    vector& operator=(vector&& other) {
        if(this == &other) {
            return *this;
        }
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        delete[] m_data;
        m_data = other.m_data;
        other.m_data = nullptr;
        return *this;
    }

    ~vector() {
        delete[] m_data;
    }
    
    void push_back(T value) {
        wwassert(m_data, "m_data is null");
        expand();
        m_data[m_size++] = move(value);
    }

    T& operator[](size_t index) {
        return m_data[index];
    }

    const T& operator[](size_t index) const {
        return m_data[index];
    }

    size_t size() const {
        return m_size;
    }

    size_t capacity() const {
        return m_capacity;
    }

    T* data() {
        return m_data;
    }

    const T* data() const {
        return m_data;
    }

    vector reversed() const {
        vector reversed_vector;
        for(size_t i = m_size - 1; i >= 0; i--) {
            reversed_vector.push_back(m_data[i]);
        }
        return reversed_vector;
    }

    T& back() {
        return m_data[m_size - 1];
    }

    const T& back() const {
        return m_data[m_size - 1];
    }

    void pop_back() {
        m_size--;
    }

    iterator begin() {
        return iterator(m_data);
    }

    iterator end() {
        return iterator(m_data + m_size);
    }

    void insert(iterator position, const T& value) {
        expand();
        wwassert(position.m_ptr >= m_data && position.m_ptr <= m_data + m_size, "position is out of range");
        for(size_t i = m_size; i > size_t(position.m_ptr - m_data); i--) {
            m_data[i] = m_data[i - 1];
        }
        m_data[position.m_ptr - m_data] = value;
        m_size++;
    }

    void erase(iterator position) {
        for(size_t i = position.m_ptr - m_data; i < m_size - 1; i++) {
            m_data[i] = m_data[i + 1];
        }
        m_size--;
    }

protected:

    void expand() {
        if(m_size == m_capacity) {
            m_capacity *= 2;
            T* new_data = new T[m_capacity];
            for(size_t i = 0; i < m_size; i++) {
                new_data[i] = move(m_data[i]);
            }
            delete[] m_data;
            m_data = new_data;
        }
    }

    size_t m_capacity;
    size_t m_size;
    T* m_data;
};

}

#endif