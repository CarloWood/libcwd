// Compile as: g++ -g -DCWDEBUG channel.cc -lcwd [-lbfd -liberty [-ldl]]
// Please see the "example-project" in the source distribution of libcwd
// for a better Real Life example.

//-------------------------------------------------------------------------------
// This block should really be part of custom header files!  Please ignore it
// and read documentation/reference-manual/preparation.html#preparation_step2 instead.
// "sys.h":
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>
#define DEBUGCHANNELS ::example		      // Where we'll put our namespace dc
// "debug.h":
#include <libcwd/debug.h>
//-------------------------------------------------------------------------------

namespace example {                           // namespace dc cannot be put in ::
  namespace dc {
    libcwd::channel_ct ghost("GHOST");  // Create our own Debug Channel
  }
}

int main(void)
{
  Debug( dc::ghost.on() );                    // Remember: don't forget to turn
  Debug( libcw_do.on() );                     // the debug Channel and Object on!  

  for (int i = 0; i < 4; ++i)
    Dout(dc::ghost, "i = " << i);             // We can write more than just
					      // "Hello World" to the ostream :)
  return 0;
}
