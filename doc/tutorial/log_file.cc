//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read doc/group__preparation.html#header_files instead.
// "debug.h"
#include <libcwd/debug.h>
//-------------------------------------------------------------------------------

#include <fstream>

int main()
{
  Debug(main_reached());
  Debug(dc::notice.on());
  Debug(libcw_do.on());

#ifdef CWDEBUG
  ofstream file;
  file.open("log");
#endif

  // Set the ostream related with libcw_do to `file':
  Debug(libcw_do.set_ostream(&file));

  Dout(dc::notice, "Hippopotamus are heavy");
}
