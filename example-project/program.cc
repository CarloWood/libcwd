#include "sys.h"
#include <iostream>		// Needed for std::cout and std::endl
#ifdef CWDEBUG
#include <cstdlib>		// Needed for getenv
#endif
#include "debug.h"

int main(void)
{
  myproject::debug::init();

  // Only turn on debug output when the environment variable SUPPRESS_DEBUG_OUTPUT is not set.
  Debug( if (getenv("SUPPRESS_DEBUG_OUTPUT") == NULL) libcw_do.on() );

  Dout(dc::custom, "This is debug output, written to a custom channel (see ./debug.h and ./debug.cc)");

  std::cout << "This program works" << std::endl;

  return 0;
}
