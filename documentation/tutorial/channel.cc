// Compile as: g++ -g -DCWDEBUG channel.cc -lcwd [-lbfd -liberty [-ldl]]
// Please see the "example-project" in the source distribution of libcwd
// for a better Real Life example.

#define _GNU_SOURCE
#include <libcw/sysd.h>
#define DEBUGCHANNELS ::example		      // Where we'll put our namespace dc
#include <libcw/debug.h>

namespace example {                           // namespace dc cannot be put in ::
  namespace dc {
    libcw::debug::channel_ct ghost("GHOST");  // Create our own Debug Channel
  }
}

int main(void)
{
  Debug( dc::ghost.on() );                    // Remember: don't forget to turn
  Debug( libcw_do.on() );                     // the debug Channel and Object on!  

  for (int i = 0; i < 4; ++i)
    Dout( dc::ghost, "i = " << i );           // We can write more than just
					      // "Hello World" to the ostream :)
  return 0;
}
