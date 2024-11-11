#ifndef _WWOS_TYPE_TRAITS_H
#define _WWOS_TYPE_TRAITS_H

#include "wwos/stdint.h"

namespace wwos {

template <typename T> struct remove_reference { using type = T; };
template <typename T> struct remove_reference<T&> { using type = T; };
template <typename T> struct remove_reference<T&&> { using type = T; };

template <typename T>
typename remove_reference<T>::type&& move(T&& arg) {
    return static_cast<typename remove_reference<T>::type&&>(arg);
}

template <typename T> T&& forward(typename remove_reference<T>::type& arg) { return static_cast<T&&>(arg); }

template <typename T> struct remove_const { using type = T; };
template <typename T> struct remove_const<const T> { using type = T; };

template <typename T> struct remove_volatile { using type = T; };
template <typename T> struct remove_volatile<volatile T> { using type = T; };

template <typename T> struct remove_cv {
    using type = typename remove_const<typename remove_volatile<T>::type>::type;
};

template <typename T> struct remove_cvref { using type = typename remove_cv<typename remove_reference<T>::type>::type; };


template <typename T> struct is_integral { static constexpr bool value = false; };
template <> struct is_integral<bool> { static constexpr bool value = true; };
template <> struct is_integral<int8_t> { static constexpr bool value = true; };
template <> struct is_integral<int16_t> { static constexpr bool value = true; };
template <> struct is_integral<int32_t> { static constexpr bool value = true; };
template <> struct is_integral<int64_t> { static constexpr bool value = true; };
template <> struct is_integral<uint8_t> { static constexpr bool value = true; };
template <> struct is_integral<uint16_t> { static constexpr bool value = true; };
template <> struct is_integral<uint32_t> { static constexpr bool value = true; };
template <> struct is_integral<uint64_t> { static constexpr bool value = true; };
template <> struct is_integral<size_t> { static constexpr bool value = true; };
         

template <typename T> struct is_pointer { static constexpr bool value = false; };
template <typename T> struct is_pointer<T*> { static constexpr bool value = true; };

template <bool, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true, T> { using type = T; };

// is vector type
template <typename T> class vector;
template <typename T> struct is_vector { static constexpr bool value = false; };
template <typename T> struct is_vector<vector<T>> { static constexpr bool value = true; };

}

#endif