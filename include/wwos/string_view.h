#ifndef _WWOS_STRING_VIEW_H
#define _WWOS_STRING_VIEW_H

#include "wwos/stdint.h"

namespace wwos {

inline size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0' && len < ~0ull) {
        len++;
    }
    return len;
}

class string;
template <typename T> class vector;

class string_view {
public:
    static constexpr size_t npos = -1;

    string_view() : m_str(nullptr), m_len(0) {}
    string_view(const char* str) : m_str(str) { m_len = strlen(str); }
    string_view(const char* str, size_t len) : m_str(str), m_len(len) {}

    string_view(const string& str);
    string_view(const vector<uint8_t>& vec);

    const char* data() const { return m_str; }
    size_t size() const { return m_len; }

    char operator[](size_t i) const { return m_str[i]; }

    string_view substr(size_t pos, size_t len) const {
        return string_view(m_str + pos, len);
    }

    bool operator==(const string_view& rhs) const {
        if (m_len != rhs.m_len) {
            return false;
        }

        for (size_t i = 0; i < m_len; i++) {
            if (m_str[i] != rhs.m_str[i]) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const string_view& rhs) const {
        return !(*this == rhs);
    }

    size_t find(char c, size_t pos = 0) const {
        for (size_t i = pos; i < m_len; i++) {
            if (m_str[i] == c) {
                return i;
            }
        }
        return npos;
    }

    size_t find_last_of(char c) const {
        for (size_t i = m_len - 1; i >= 0; i--) {
            if (m_str[i] == c) {
                return i;
            }
        }
        return npos;
    }

private:
    const char* m_str;
    size_t m_len;
};


inline bool stoi(const string_view& str, int32_t& out) {
    int result = 0;
    bool neg = false;
    for (size_t i = 0; i < str.size(); i++) {
        if(i == 0 && str[i] == '-') {
            neg = true;
            continue;
        }
        if(str[i] < '0' || str[i] > '9') {
            return false;
        }
        result = result * 10 + (str[i] - '0');
    }
    if(neg && str.size() == 1) {
        return false;
    }
    if(neg) {
        result = -result;
    }
    out = result;
    return true;
}

}

#endif