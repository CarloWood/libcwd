#include "sys.h"
#include "debug.h"

namespace example {
  namespace debug {
    namespace channels {
      namespace dc {
	::libcw::debug::channel_ct warp("WARP");
      }
    }
  }
}

int main(void)
{
#if !CWDEBUG_ALLOC || !CWDEBUG_LOCATION
  DoutFatal(dc::fatal, "Expected Failure.");
#endif
  Debug( check_configuration() );
  libcw::debug::make_all_allocations_invisible_except(NULL);
  Debug( libcw_do.on() );
  Debug( dc::warp.on() );
  Debug( dc::notice.on() );

  Dout(dc::notice, "Debug channel Test.");
  Dout(dc::warp, "Custom channel Test.");

  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
  Debug( list_channels_on(libcw_do) );
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  Debug( list_channels_on(libcw_do) );

#if CWDEBUG_LOCATION
  Dout(dc::bfd, "bfd Testing");
#else
  Dout(dc::notice, "bfd Testing disabled");
#endif
  Dout(dc::debug, "debug Testing");
  Dout(dc::malloc, "malloc Testing");
  Dout(dc::notice, "notice Testing");
  Dout(dc::system, "system Testing");
  Dout(dc::warning, "warning Testing");

  Dout( dc::notice|dc::system|dc::warning, "Hello World" );
  Debug( dc::notice.off() );
  Dout( dc::notice|dc::system|dc::warning, "Hello World" );
  Debug( dc::system.off() );
  Dout( dc::notice|dc::system|dc::warning, "Hello World" );
  Debug( dc::warning.off() );
  Dout( dc::notice|dc::system|dc::warning, "Hello World (not)" );

  exit(0);
}
