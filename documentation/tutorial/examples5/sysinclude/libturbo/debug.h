#include <libcw/debug.h>

namespace turbo {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace libcw::debug;
	using namespace libcw::debug::channels::dc;
	extern channel_ct turbo;
	extern channel_ct foobar;
      }
    }
  }
}

namespace libcw {
  namespace debug {
    using namespace turbo::debug;
  }
}
