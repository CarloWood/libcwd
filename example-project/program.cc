#include "sys.h"
#include <iostream>		// Needed for std::cout and std::endl
#ifdef CWDEBUG
#include <cstdlib>		// Needed for getenv
#endif
#include "debug.h"

int main(void)
{
  // You want this, unless you mix streams output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  std::ios::sync_with_stdio(false);

  // This will warn you when you are using header files that do not belong to the
  // shared libcwd object that you linked with.
  Debug( check_configuration() );

  // Turn on the custom channel.
  Debug( dc::custom.on() );

  // Only turn on debug output when the environment variable SUPPRESS_DEBUG_OUTPUT is not set.
  Debug( if (getenv("SUPPRESS_DEBUG_OUTPUT") == NULL) libcw_do.on() );

  Dout(dc::custom, "This is debug output, written to a custom channel (see ./debug.h and ./debug.cc)");

  std::cout << "This program works" << std::endl;

  return 0;
}
