#ifdef HAVE_CONFIG_H		// This is just an example of what you could do
#include "config.h"		//   when using autoconf for your project.
#endif
#ifdef CWDEBUG			// This is needed so that others can compile
#include <libcw/sysd.h> 	//   your application without having libcwd installed.
#endif
