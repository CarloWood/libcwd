#define CW_DEBUG 1

#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#include "libcwd/private_struct_TSD.h"
#endif
#include "libcwd/macro_ForAllDebugObjects.h"

// Required for threadsafe.
#define ASSERT(x) LIBCWD_ASSERT(x)
#define DEBUG_ONLY(x) x
