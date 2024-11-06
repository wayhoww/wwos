#ifndef _WWOS_STRING_H
#define _WWOS_STRING_H

#include "wwos/stdint.h"
#include "wwos/vector.h"

namespace wwos {

    class string: public vector<uint8_t> {
    public:
        constexpr static size_t npos = -1;

        string() {
            vector<uint8_t>::push_back(0);
        }

        string(const char* s) {
            vector<uint8_t>::push_back(0);
            while(*s != '\0') {
                push_back(*s);
                s++;
            }
        }

        string(const char* s, size_t len) {
            vector<uint8_t>::push_back(0);
            for(size_t i = 0; i < len; i++) {
                push_back(s[i]);
            }
        }

        string(const string& other) {
            vector<uint8_t>::push_back(0);
            for(size_t i = 0; i < other.size(); i++) {
                push_back(other[i]);
            }
        }

        string& operator=(const string& other) {
            clear();
            for(size_t i = 0; i < other.size(); i++) {
                push_back(other[i]);
            }
            return *this;
        }

        string& operator+=(const string& other) {
            for(size_t i = 0; i < other.size(); i++) {
                push_back(other[i]);
            }
            return *this;
        }

        string& push_back(char c) {
            this->m_data[this->m_size - 1] = c;
            vector<uint8_t>::push_back(0);
            return *this;
        }

        string reversed() const {
            string reversed_string;
            for(int64_t i = size() - 1; i >= 0; i--) {
                reversed_string.push_back((*this)[i]);
            }
            return reversed_string;
        }

        iterator end() {
            return iterator(m_data + m_size - 1);
        }

        bool operator==(const string& other) const {
            if(size() != other.size()) {
                return false;
            }
            for(size_t i = 0; i < size(); i++) {
                if((*this)[i] != other[i]) {
                    return false;
                }
            }
            return true;
        }

        bool operator!=(const string& other) const {
            return !(*this == other);
        }

        string operator+(const string& other) const {
            string result = *this;
            result += other;
            return result;
        }

        void clear() {
            m_data[0] = 0;
            m_size = 1;
        }

        size_t size() const {
            return m_size - 1;
        }

        const char* c_str() const {
            return (const char*) m_data;
        }

        size_t find(char c, size_t pos = 0) const {
            for(size_t i = pos; i < size(); i++) {
                if((*this)[i] == c) {
                    return i;
                }
            }
            return npos;
        }

        size_t find_last_of(char c) const {
            for(int64_t i = size() - 1; i >= 0; i--) {
                if((*this)[i] == c) {
                    return i;
                }
            }
            return npos;
        }

        string substr(size_t pos, size_t len) const {
            string out;
            for(size_t i = pos; i < pos + len; i++) {
                out.push_back((*this)[i]);
            }
            return out;
        }
    };
}

#endif