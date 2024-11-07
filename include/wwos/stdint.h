#ifndef _WWOS_STDINT_H
#define _WWOS_STDINT_H

namespace wwos {
    
using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;

using int8_t = signed char;
using int16_t = signed short;
using int32_t = signed int;
using int64_t = signed long long;

using size_t = unsigned long;

}

#ifndef WWOS_HOST

namespace std {
#ifdef __clang__
    enum struct align_val_t: wwos::size_t {};
#endif
}

#endif

#endif