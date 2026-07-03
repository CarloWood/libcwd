#ifndef EXAMPLE_DEBUG_H
#define EXAMPLE_DEBUG_H

#ifdef CWDEBUG

#ifndef LIBCWD_DEBUG_CHANNELS   // This test is only necessary in libraries.
#define LIBCWD_DEBUG_CHANNELS example::debug::channels
#endif
#include <libbooster/debug.h>     // Note that these will include
#include <libturbo/debug.h>       //   libcwd/debug.h for us.

namespace example {
  namespace debug {
    namespace channels {
      namespace dc {
        using namespace libcwd;         // For class Channel

        // The list of debug channel namespaces of the libraries that we use:
        // (These two already include libcwd::channels::dc)
        using namespace ::booster::debug::channels::dc;
        using namespace ::turbo::debug::channels::dc;

        // Our own debug channels:
        extern Channel elephant;
        extern Channel cat;
        extern Channel mouse;
        extern Channel owl;

        // When the libraries use the same name for any debug channels,
        // then here is the place to `hide' these channels by redefining them.
        // For example, if `libbooster' defined `notice' too (as does libcwd)
        // then we have to redefine it again:
        using libcwd::channels::dc::notice;

      } // namespace dc
    } // namespace channels
  } // namespace debug
} // namespace example

// The following is only necessary for libraries.
// Libraries should not use Dout() et al. in their own header files,
// instead we define our own macros here for use in those header files:
#define MyLibHeaderDout(cntrl, ...) \
      LibcwDout(example::debug::channels, ::libcwd::libcw_do, cntrl, __VA_ARGS__)
#define MyLibHeaderDoutFatal(cntrl, ...) \
      LibcwDoutFatal(example::debug::channels, ::libcwd::libcw_do, cntrl, __VA_ARGS__)

#else // !CWDEBUG

// This is needed so people who don't have libcwd installed can still compile it.
// The file "nodebug.h" is provided in the libcwd source tree and needs to be included
// in your own package.
#include "nodebug.h"
#define MyLibHeaderDout(cntrl, ...)
#define MyLibHeaderDoutFatal(cntrl, ...) LibcwDoutFatal(::std, /*nothing*/, cntrl, __VA_ARGS__)

#endif // !CWDEBUG

#endif // EXAMPLE_DEBUG_H
