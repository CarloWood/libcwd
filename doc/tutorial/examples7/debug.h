#ifndef DEBUG_H
#define DEBUG_H
 
#ifndef CWDEBUG
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, x)
#define AllocTag(p, x)
#define ASSERT(x)
#define Debug(x)
#define Dout(a, b)
#define DoutFatal(a, b) LibcwDoutFatal(::std, , a, b)
#define ForAllDebugChannels(STATEMENT)
#define ForAllDebugObjects(STATEMENT)
#define LibcwDebug(dc_namespace, x)
#define LibcwDout(a, b, c, d)
#define LibcwDoutFatal(a, b, c, d) do { ::std::cerr << d << ::std::endl; exit(-1); } while(1)
#define NEW(x) new x
#define set_alloc_checking_off()
#define set_alloc_checking_on()
#else
#include <libcwd/debug.h>
#endif

#endif
