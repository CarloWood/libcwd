#include "sys.h"
#include <libcwd/debug.h>

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  Dout(dc::notice, "Basic Test.");

  EXIT(0);
}
