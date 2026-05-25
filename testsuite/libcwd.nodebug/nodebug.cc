#include "sys.h"
#include <libcwd/debug.h>

int main()
{
  Debug(main_reached());
  Debug(libcw_do.on());
#if CWDEBUG_LOCATION
  Debug( dc::bfd.on() );

  libcwd::location_ct loc(&&current_location);
current_location:
  Dout(dc::bfd, "This is printed from " << loc);
#endif
}
