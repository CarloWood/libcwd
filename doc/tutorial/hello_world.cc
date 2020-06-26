//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read documentation/reference-manual/preparation.html#preparation_step2 instead.
// "sys.h":
#ifndef _GNU_SOURCE		// g++ 3.0 and higher already defines this.
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>		// This must be the first header file
// "debug.h"
#include <libcwd/debug.h>
//-------------------------------------------------------------------------------

int main()
{
  Debug( dc::notice.on() );		// Turn on the NOTICE Debug Channel
  Debug( libcw_do.on() );		// Turn the default Debug Object on

  Dout( dc::notice, "Hello World" );
}
