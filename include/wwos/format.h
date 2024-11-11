#ifndef _WWOS_FORMAT_H
#define _WWOS_FORMAT_H

#include "wwos/assert.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string.h"
#include "wwos/string_view.h"
#include "wwos/type_traits.h"
#include "wwos/string_view.h"

namespace wwos {

template <size_t i, size_t count, typename T, typename... Ts>
auto get_element(const T& elem, const Ts&... args) {
    if constexpr(i == count - 1) {
        return elem;
    } else {
        return get_element<i + 1, count>(forward<Ts...>(args...));
    }
}

template <typename C, typename T>
void noalloc_generic_format_element(
    C& out,
    string_view format_string, 
    const typename enable_if<
        is_integral<typename remove_cvref<T>::type>::value || is_pointer<typename remove_cvref<T>::type>::value,
        typename remove_cvref<T>::type
    >::type& elem_in
) {  
    auto elem = elem_in;

    constexpr bool is_ptr = is_pointer<typename remove_cvref<T>::type>::value;

    if constexpr (is_ptr) {
        out.put('0');
        out.put('p');
        for(int i = sizeof(elem) * 2 - 1; i >= 0; i--) {
            out.put("0123456789abcdef"[(reinterpret_cast<uint64_t>(elem) >> (i * 4)) & 0xf]);
        }
    } else {
        if(format_string == "x") {
            out.put('0');
            out.put('x');
            for(int i = sizeof(elem) * 2 - 1; i >= 0; i--) {
                out.put("0123456789abcdef"[(elem >> (i * 4)) & 0xf]);
            }
        } else {
            if(elem == 0) {
                out.put('0');
                return;
            }

            if(elem < 0) {
                out.put('-');
                elem = -elem;
            }

            uint8_t buffer[32]; // maximum uint64_t is 20 digits
            int len = 0;
            do {
                buffer[len++] = '0' + elem % 10;
                elem /= 10;
            } while(elem != 0);

            for(int i = len - 1; i >= 0; i--) {
                out.put(buffer[i]);
            }
        }
    }
}

template <typename C, typename T = string_view> 
inline void noalloc_generic_format_element(
    C& out, string_view format_string, string_view elem
) { 
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
        if(lastchar == 'r') {
            for(uint64_t i = 0; i < elem.size(); i++) {
                out.put(elem[i]);
            }
            for(int64_t i = 0; i < width - int64_t(elem.size()); i++) {
                out.put(' ');
            }
        } else {
            for(int64_t i = 0; i < width - int64_t(elem.size()); i++) {
                out.put(' ');
            }
            for(size_t i = 0; i < elem.size(); i++) {
                out.put(elem[i]);
            }
        }
    } else {
        for(size_t i = 0; i < elem.size(); i++) {
            out.put(elem[i]);
        }
    } 
}

template <typename C, typename T = const char*>
inline void noalloc_generic_format_element(
    C& out, string_view format_string, const char* elem
) { 
    noalloc_generic_format_element(out, format_string, string_view(elem));
}

template <typename C, typename T = const string&>
inline void noalloc_generic_format_element(
    C& out, string_view format_string, const string& elem
) { 
    noalloc_generic_format_element(out, format_string, string_view(elem));
}


template <typename C, typename T>
void noalloc_generic_format_element(
    C& out,
    string_view format_string, 
    const typename enable_if<
        !is_integral<typename remove_cvref<T>::type>::value && is_vector<typename remove_cvref<T>::type>::value, 
        typename remove_cvref<T>::type
    >::type& elem
) {  
    out.put('[');
    for(size_t i = 0; i < elem.size(); i++) {
        noalloc_generic_format_element<C, decltype(elem[i])>(out, format_string, elem[i]);
        if(i + 1 < elem.size()) {
            out.put(',');
            out.put(' ');
        }
    }
    out.put(']');
}

template <size_t i, typename C, typename T, typename... Ts>
void noalloc_generic_format_element_at(C& out, string_view format_string, const T& elem, const Ts&... args) {
    if constexpr (i == 0) {
        noalloc_generic_format_element<C, T>(out, format_string, elem);
    } else {
        noalloc_generic_format_element_at<i - 1, C>(out, format_string, args...);
    }
}


template <size_t i, size_t count, typename C, typename... Ts>
void noalloc_generic_dynamic_format_element_at(C& out, size_t i_dynamic, string_view format_string, const Ts&... args) {
    if constexpr (i < count) {
        if(i_dynamic == i) {
            noalloc_generic_format_element_at<i, C, Ts...>(out, format_string, args...);
        } else {
            noalloc_generic_dynamic_format_element_at<i + 1, count, C, Ts...>(out, i_dynamic, format_string, args...);
        }
    } else {
        wwassert(false, "index out of range");
    }
}


template <typename C, typename... Ts>
void noalloc_generic_printf(C& out, string_view format, const Ts&... args) {
    size_t i = 0;
    size_t p_format = 0;
    while(i < format.size()) {
        if(format[i] == '{') {
            if(i + 1 < format.size() && format[i + 1] == '{') {
                out.put('{');
                i += 2;
            } else {
                size_t j = i + 1;
                size_t pos_of_colon = format.size();
                while(j < format.size() && format[j] != '}') {
                    if(format[j] == ':' && pos_of_colon == format.size())  pos_of_colon = j;
                    j++;
                }
                if(j == format.size()) {
                    out.put('{');
                    i++;
                } else {
                    if(pos_of_colon == format.size()) {
                        noalloc_generic_dynamic_format_element_at<0, sizeof...(args), C, Ts...>(out, p_format, "", args...);
                    } else {
                        string_view format_string(format.data() + pos_of_colon + 1, j - pos_of_colon - 1);
                        noalloc_generic_dynamic_format_element_at<0, sizeof...(args), C, Ts...>(out, p_format, format_string, args...);
                    }
                    i = j + 1;
                    p_format++;
                }
                
            }
        } else {
            out.put(format[i]);
            i++;
        }
    }
}

struct print_to_putchar { void put(uint8_t c) { putchar(c); } };

template <typename... Ts>
void printf(string_view formats, const Ts&... args) {
    print_to_putchar out;
    noalloc_generic_printf(out, formats, args...);
}

struct print_to_kputchar { void put(uint8_t c) { putchar(c); } };

template <typename... Ts>
void kprintf(string_view formats, const Ts&... args) {
    print_to_kputchar out;
    noalloc_generic_printf(out, formats, args...);
}

struct print_to_string { 
    string out;
    void put(uint8_t c) { out.push_back(c); } 
};

template <typename... Ts>
string format(string_view formats, const Ts&... args) {
    print_to_string out;
    noalloc_generic_printf(out, formats, args...);
    return out.out;
}

template <typename... Ts>
void trigger_log(string_view file, int line, string_view msg, const Ts&... args) {
    printf("{}:{} - ", file, line);
    printf(msg, args...);
    printf("\n");
}

#ifdef WWOS_LOG
#define wwlog(msg) { { wwos::trigger_log(__FILE__, __LINE__, msg); } }
#define wwfmtlog(msg, ...) { { wwos::trigger_log(__FILE__, __LINE__, msg, __VA_ARGS__); } }
#else
#define wwlog(msg)
#define wwfmtlog(msg, ...)
#endif

}

#endif