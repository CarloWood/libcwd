#ifndef DEBUG_H
#define DEBUG_H

#ifndef DEBUG
#include "nodebug.h"
#else // DEBUG

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

#endif // DEBUG

#endif // DEBUG_H
