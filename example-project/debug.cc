#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG

namespace myproject {
  namespace debug {
    namespace channels {
      namespace dc {

        // Add new debug channels here.
	::libcw::debug::channel_ct const custom("CUSTOM");

      }
    }
  }
}

#endif // DEBUG
