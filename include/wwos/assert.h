
#ifndef _WWOS_ASSERT_H

#include "wwos/string_view.h"

#ifdef WWOS_HOST
#include <iostream>
#include <cassert>
#endif


namespace wwos {
    class string_view;

    [[noreturn]] void trigger_assert(string_view, string_view file, int line, string_view msg);
    void trigger_mark(string_view file, int line, string_view msg);
    void trigger_log(string_view file, int line, string_view msg);
}


#ifdef WWOS_HOST
#define wwassert(expr, msg) assert(expr)
#define wwmark(msg) { { std::cerr << msg << std::endl; } }
#ifdef WWOS_LOG
#define wwlog(msg) { { std::cerr << msg << std::endl; } }
#define wwfmtlog(msg, ...) { { std::cerr << wwos::format(msg, __VA_ARGS__) << std::endl; } }
#endif
#else
#define wwassert(expr, msg) { if (!(expr)) { wwos::trigger_assert(#expr, __FILE__, __LINE__, msg); } }
#define wwmark(msg) { { wwos::trigger_mark(__FILE__, __LINE__, msg); } }
#ifdef WWOS_LOG
#define wwlog(msg) { { wwos::trigger_log(__FILE__, __LINE__, msg); } }
#define wwfmtlog(msg, ...) { { wwos::trigger_log(__FILE__, __LINE__, wwos::format(msg, __VA_ARGS__)); } }
#endif
#endif

#ifndef WWOS_LOG
#define wwlog(msg)
#define wwfmtlog(msg, ...)
#endif

#endif