#ifndef EXAMPLE_DEBUG_H
#define EXAMPLE_DEBUG_H

#ifdef CWDEBUG

#define DEBUGCHANNELS ::example::debug::channels
#include <libcwd/debug.h>

namespace example {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace ::libcwd::channels::dc;
	extern ::libcwd::channel_ct warp;
      }
    }
  }
}

#else

#define Dout(cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::std, /*nothing*/, cntrl, data)
#define Debug(x)

#endif

#endif // EXAMPLE_DEBUG_H
