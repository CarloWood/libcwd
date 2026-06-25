//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read doc/group__preparation.html#header_files instead.
// "debug.h"
#include <libcwd/debug.h>
//-------------------------------------------------------------------------------

int main()
{
  Debug(main_reached());        // Mandatory call to notify the library that main() was reached.
  Debug(dc::notice.on());       // Turn on the NOTICE Debug Channel.
  Debug(libcw_do.on());         // Turn the default Debug Object on.

  Dout(dc::notice, "Hello World");
}
