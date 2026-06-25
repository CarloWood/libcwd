// Compile as: g++ -g -DCWDEBUG channel.cc -lcwd
// Please see the "example-project" in the source distribution of libcwd
// for a better Real Life example.

//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read doc/group__preparation.html#header_files instead.
// "debug.h"
#define DEBUGCHANNELS example           // Where we'll put our namespace dc.
namespace DEBUGCHANNELS { }
#include <libcwd/debug.h>
//-------------------------------------------------------------------------------

namespace DEBUGCHANNELS::dc {           // Note that `dc` cannot be put in ::.
libcwd::Channel ghost("GHOST");         // Create our own Debug Channel.
} // namespace DEBUGCHANNELS::dc

int main()
{
  Debug(main_reached());                // Mandatory call to notify the library that main() was reached.
  Debug(dc::ghost.on());                // Remember: don't forget to turn
  Debug(libcw_do.on());                 // the debug Channel and Object on!

  for (int i = 0; i < 4; ++i)
    Dout(dc::ghost, "i = " << i);       // We can write more than just
					// "Hello World" to the ostream :)
}
