#ifndef ALLOCTAG_DEBUG_H
#define ALLOCTAG_DEBUG_H

#include <libcw/debug.h>

#if !CWDEBUG_ALLOC
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, x)
#define AllocTag(p, x)
#define NEW(x) new x
#endif

#endif // ALLOCTAG_DEBUG_H

