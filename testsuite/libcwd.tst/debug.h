#ifndef EXAMPLE_DEBUG_H
#define EXAMPLE_DEBUG_H

#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::example::debug::channels
#endif
#include <libcw/debug.h>

namespace example {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace ::libcw::debug::channels::dc;
	extern ::libcw::debug::channel_ct const warp;
      };
    };
  };
};

#undef Dout
#undef Dout_vform
#undef DoutFatal
#ifdef DEBUG
#define Dout(cntrl, data) LibcwDout(::example::debug::channels, ::libcw::debug::libcw_do, cntrl, data)
#define Dout_vform(cntrl, format, vl) LibcwDout_vform(::example::debug::channels, ::libcw::debug::libcw_do, cntrl, format, vl)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::example::debug::channels, ::libcw::debug::libcw_do, cntrl, data)
#else
#define Dout(cntrl, data)
#define Dout_vform(cntrl, format, vl)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::std, /*nothing*/, cntrl, data)
#endif

#endif // EXAMPLE_DEBUG_H
