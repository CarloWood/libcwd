// Compile as: g++ -g -DCWDEBUG channel.cc -lcwd
// Please see the "example-project" in the source distribution of libcwd
// for a better Real Life example.

//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read doc/group__preparation.html#header_files instead.
// "debug.h"
#define LIBCWD_DEBUG_CHANNELS example   // Where we'll put our namespace dc.
                                        // Note this may NOT start with ::.
#include <libcwd/debug.h>
//-------------------------------------------------------------------------------

NAMESPACE_DEBUG_CHANNELS_START          // In this case this is example::channels
Channel ghost("GHOST");                 // Create our own Debug Channel.
NAMESPACE_DEBUG_CHANNELS_END

int main()
{
  Debug(NAMESPACE_DEBUG::init());       // Mandatory call to notify the library that
                                        // main() was reached, check that the correct
                                        // headers are being used and to read the rcfile.

  // Lets not forget to turn the debug Channel and Object on!
  Debug(if (!dc::ghost.is_on()) dc::ghost.on()); // Might already be on due to rcfile.
  Debug(libcw_do.on());

  for (int i = 0; i < 4; ++i)
    Dout(dc::ghost, "i = " << i);       // We can write more than just
                                        // "Hello World" to the ostream :)
}
