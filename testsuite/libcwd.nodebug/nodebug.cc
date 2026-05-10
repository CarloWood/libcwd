#include "sys.h"
#include <libcwd/debug.h>

int main()
{
  DoutFatal(dc::fatal, "Expected Failure.");

  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
#if CWDEBUG_LOCATION
  Debug( dc::bfd.on() );
#endif

  int* p = new int [100];

  delete [] p;

  return 0;
}
