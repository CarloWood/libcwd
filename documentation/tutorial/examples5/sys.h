// This is just an example of what you could do when using autoconf for your project.
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
// This is needed so that others can compile your application without having libcwd installed.
#ifdef CWDEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE		// Libcwd uses GNU extensions.
#endif
#include <libcwd/sys.h>
#endif
