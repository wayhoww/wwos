
#ifndef _WWOS_ASSERT_H

#include "wwos/string_view.h"

#ifdef WWOS_HOST
#include <iostream>
#include <cassert>
#endif


namespace wwos {
    class string_view;

    [[noreturn]] void trigger_assert(string_view, string_view file, int line, string_view msg);
}


#ifdef WWOS_HOST
#define wwassert(expr, msg) assert(expr)
#else
#define wwassert(expr, msg) { if (!(expr)) { wwos::trigger_assert(#expr, __FILE__, __LINE__, msg); } }
#endif

#endif