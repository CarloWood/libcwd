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

#include <fstream>

int main(void)
{
  Debug( dc::notice.on() );
  Debug( libcw_do.on() );

#ifdef CWDEBUG
  ofstream file;
  file.open("log");
#endif

  // Set the ostream related with libcw_do to `file':  
  Debug( libcw_do.set_ostream(&file) );

  Dout(dc::notice, "Hippopotamus are heavy");

  return 0;
}
