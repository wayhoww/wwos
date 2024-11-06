#include "wwos/string_view.h"
#include "wwos/string.h"


namespace wwos {

    string_view::string_view(const string& str) : m_str(str.c_str()), m_len(str.size()) {}
    string_view::string_view(const vector<uint8_t>& vec) : m_str(reinterpret_cast<const char*>(vec.data())), m_len(vec.size()) {}

}