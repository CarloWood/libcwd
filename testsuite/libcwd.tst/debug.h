#ifndef EXAMPLE_DEBUG_H
#define EXAMPLE_DEBUG_H

#ifdef CWDEBUG

//#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::example::debug::channels
//#endif
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

#undef Dout
#undef DoutFatal

#define Dout(cntrl, data) LibcwDout(::example::debug::channels, ::libcwd::libcw_do, cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::example::debug::channels, ::libcwd::libcw_do, cntrl, data)

#else

#define Dout(cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::std, /*nothing*/, cntrl, data)
#define Debug(x)

#endif

#endif // EXAMPLE_DEBUG_H
