#ifdef CWDEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>
#endif

// Bug workaround for libstdc++, avoid warnings like
// /usr/include/g++-3/iostream.h:253:5: "_G_CLOG_CONFLICT" is not defined
#include <stdio.h>
#ifndef _G_CLOG_CONFLICT
#define _G_CLOG_CONFLICT 0
#endif
#ifndef _G_HAS_LABS
#define _G_HAS_LABS 1
#endif
