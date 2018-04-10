#include "sys.h"                // Include this header at the top of all your source files.
#include "debug.h"              // Needed when this compilation unit uses libcwd.
#include <iostream>		// Needed for std::cout and std::endl

int main()
{
  // 'myproject::debug' is a namespace that was defined in debug.h.
  Debug(myproject::debug::init());

#ifdef CWDEBUG
  if (!DEBUGCHANNELS::dc::custom.is_on())
    std::cout << "Turn dc::custom on in your ~/.libcwdrc file by adding \"channels_on=custom\"." << std::endl;
#endif

  // Write debug output to channel 'custom'.
  Dout(dc::custom, "This is debug output, written to a custom channel (see ../debug.h and ./debug.cpp)");
}
