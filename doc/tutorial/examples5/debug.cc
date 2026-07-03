#include "debug.h"

#ifdef CWDEBUG
using namespace libcwd;

namespace example {
  namespace debug {
    namespace channels {
      namespace dc {
        Channel elephant("DUMBO");
        Channel cat("FELIX");
        Channel mouse("HILDAGO");
        Channel owl("EINSTEIN");
      }
    }
  }
}
#endif
