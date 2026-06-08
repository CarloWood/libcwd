#include <libcwd/debug.h>

namespace turbo {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace libcwd;
	using namespace libcwd::channels::dc;
	extern Channel turbo;
	extern Channel foobar;
      }
    }
  }
}

namespace libcwd {
  using namespace turbo::debug;
}
