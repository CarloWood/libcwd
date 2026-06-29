#include "sys.h"                // Include this header at the top of all your source files.

#include <iostream>		// Needed for std::cout and std::endl
#include "debug.h"              // Needed when this compilation unit uses libcwd.

int main()
{
  // NAMESPACE_DEBUG was set to the namespace 'myproject::debug',
  // a namespace that was defined in "debug.h".
  Debug(NAMESPACE_DEBUG::init());

#ifdef CWDEBUG
  if (!LIBCWD_DEBUGCHANNELS::dc::custom.is_on())
    std::cout << "Turn dc::custom on in your ~/.libcwdrc file by adding \"channels_on=custom\"." << std::endl;
#else
  std::cerr << "This program was compiled without libcwd. Was it detected during configuration? Did you forget to *install* libcwd before running configure on this project?" << std::endl;
#endif

  // Write debug output to channel 'custom'.
  Dout(dc::custom, "This is debug output, written to a custom channel (see ../debug.h and ./debug.cpp)");

  // Write information from the config.h file.
  Dout(dc::notice, "The following is defined in config.h, which was automatically included by sys.h");
  std::cout << "Project name: " << config::PROJECT_NAME << " version " << config::PROJECT_VERSION << ".\n";
  std::cout << "Project description: " << config::PROJECT_DESCRIPTION << '\n';

#if USE_EXAMPLE && defined(USE_EXAMPLE42) && (USE_EXAMPLE42 == 42)
  std::cout << "ExampleOption was most definitely set!\n";
#endif
}
