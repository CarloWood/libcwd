// EXAMPLE: If you use autoconf you can add the following here:
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
// EXAMPLE: Here you could add stuff like:
#if defined(WANTSDEBUGGING) && defined(HAVE_LIBCWD_BLAHBLAH)
#define CWDEBUG
#endif
// The following is the mandatory part of the custom "sys.h".
// This must be included *before* any system header file is included.
#ifdef CWDEBUG
#include <libcw/sysd.h>
#endif
