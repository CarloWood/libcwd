//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read documentation/html/preparation.html#preparation_step2 instead.
// "sys.h":
#ifndef _GNU_SOURCE		// g++ 3.0.3 and higher already define this.
#define _GNU_SOURCE
#endif
#include <libcw/sysd.h>		// This must be the first header file
// "debug.h"
#include <libcw/debug.h>
//-------------------------------------------------------------------------------

int main(void)
{
  Debug( dc::notice.on() );		// Turn on the NOTICE Debug Channel
  Debug( libcw_do.on() );		// Turn the default Debug Object on

  Dout( dc::notice, "Hello World" );

  return 0;
}

