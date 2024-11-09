#ifndef _WWOS_FORMAT_H
#define _WWOS_FORMAT_H

#include "wwos/assert.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string.h"
#include "wwos/string_view.h"
#include "wwos/type_traits.h"

namespace wwos {

template <typename T>
inline string to_string(T value, int base = 10) {
    string result;
    do {
        T digit = value % base;
        value /= base;
        result.push_back(digit < 10 ? '0' + digit : 'a' + digit - 10);
    } while(value != 0);
    return result.reversed();
}

template <typename T>
string format_element(
    string_view format_string, 
    typename enable_if<is_integral<typename remove_cvref<T>::type>::value, typename remove_cvref<T>::type>::type elem
) {  
    if(format_string == "x") {
        return string("0x") + to_string(elem, 16);
    } else {
        return to_string(elem);
    }
}

template <typename T = string> inline string format_element(string_view format_string, string elem) { 
    if(format_string.size() > 0) {
        auto lastchar = format_string[format_string.size() - 1];
        if(lastchar == 'l' || lastchar == 'r') {
            format_string = format_string.substr(0, format_string.size() - 1);
        } else {
            lastchar = 'r';
        }
        int width = 0;
        bool succ = stoi(format_string, width);
        wwassert(succ, "invalid width");
        string out;
        if(lastchar == 'r') {
            out = elem;
            for(int64_t i = 0; i < width - int64_t(elem.size()); i++) {
                out.push_back(' ');
            }
        } else {
            for(int64_t i = 0; i < width - int64_t(elem.size()); i++) {
                out.push_back(' ');
            }
            out += elem;
        }
        return out;
    } else {
        return elem;
    } 
}
template <typename T = const char*> inline string format_element(string_view format_string, const char* elem) { 
    return format_element<string>(format_string, string(elem));
}
template <typename T = string_view> inline string format_element(string_view format_string, string_view elem) { 
    return format_element<string>(format_string, string(elem.data(), elem.size()));
}


template <size_t i, size_t count, typename T, typename... Ts>
auto get_element(T&& elem, Ts&&... args) {
    if constexpr(i == count - 1) {
        return elem;
    } else {
        return get_element<i + 1, count>(forward<Ts...>(args...));
    }
}

template <size_t i, typename T, typename... Ts>
string format_element_at(string_view format_string, T&& elem, Ts&&... args) {
    if constexpr (i == 0) {
        return format_element<T>(format_string, forward<T>(elem));
    } else {
        return format_element_at<i - 1>(format_string, forward<Ts>(args)...);
    }
}

template <size_t i, size_t count, typename... Ts>
string dynamic_format_element_at(size_t i_dynamic, string_view format_string,  Ts&&... args) {
    if constexpr (i < count) {
        if(i_dynamic == i) {
            return format_element_at<i, Ts...>(format_string, forward<Ts>(args)...);
        } else {
            return dynamic_format_element_at<i + 1, count, Ts...>(i_dynamic, format_string, forward<Ts>(args)...);
        }
    } else {
        wwassert(false, "index out of range");
    }
}

template<typename... Ts>
string format(const string_view& format, Ts&&... args) {
    string out;

    size_t i = 0;
    size_t p_format = 0;
    while(i < format.size()) {
        if(format[i] == '{') {
            if(i + 1 < format.size() &&  format[i + 1] == '{') {
                out.push_back('{');
                i += 2;
            } else {
                size_t j = i + 1;
                size_t pos_of_colon = format.size();
                while(j < format.size() && format[j] != '}') {
                    if(format[j] == ':' && pos_of_colon == format.size())  pos_of_colon = j;
                    j++;
                }
                if(j == format.size()) {
                    out.push_back('{');
                    i++;
                } else {
                    if(pos_of_colon == format.size()) {
                        out += dynamic_format_element_at<0, sizeof...(args), Ts...>(p_format, "", forward<Ts>(args)...);
                    } else {
                        string_view format_string(format.data() + pos_of_colon + 1, j - pos_of_colon - 1);
                        out += dynamic_format_element_at<0, sizeof...(args), Ts...>(p_format, format_string, forward<Ts>(args)...);
                    }
                    i = j + 1;
                    p_format++;
                }
                
            }
        } else {
            out.push_back(format[i]);
            i++;
        }
    }
    return out;
    
    return string(format.data(), format.size());
}

template <typename... Ts>
void printf(const string& formats, Ts&&... args) {
    print(format<Ts...>(formats, forward<Ts>(args)...));
}

}

#endif