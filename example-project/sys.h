// sys.h
//
// This header file is included at the top of every source file,
// before any other header file.
// It is intended to add defines that are needed globally and
// to work around Operating System dependend incompatibilities.

// EXAMPLE: If you use autoconf you can add the following here.
// #ifdef HAVE_CONFIG_H
// #include "config.h"
// #endif

// EXAMPLE: You could add stuff like this here too
// (Otherwise add -DCWDEBUG to your CFLAGS).
// #if defined(WANTSDEBUGGING) && defined(HAVE_LIBCWD_BLAHBLAH)
// #define CWDEBUG
// #endif

// The following is the libcwd related mandatory part.
// It must be included before any system header file is included!
#ifdef CWDEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>
#endif
