#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG

namespace myproject {		// >
  namespace debug {		//  >--> This part must match DEBUGCHANNELS, see debug.h
    namespace channels {	// >

      namespace dc {

        // Add new debug channels here.
	libcw::debug::channel_ct custom("CUSTOM");

      } // namespace dc

    } // namespace DEBUGCHANNELS
  }
}

#endif // CWDEBUG
