#include <libcwd/debug.h>

namespace turbo {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace libcwd;
	using namespace libcwd::channels::dc;
	extern channel_ct turbo;
	extern channel_ct foobar;
      }
    }
  }
}

namespace libcwd {
  using namespace turbo::debug;
}
