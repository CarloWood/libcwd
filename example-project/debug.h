#ifndef DEBUG_H
#define DEBUG_H

#ifndef CWDEBUG
#include "nodebug.h"

#else // CWDEBUG

#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::myproject::debug::channels
#endif
#include <libcw/debug.h>

namespace myproject {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace ::libcw::debug::channels::dc;

	// Add new debug channels here.
	extern ::libcw::debug::channel_ct const custom;

      }
    }
  }
}

#endif // CWDEBUG

#endif // DEBUG_H
