#include "sys.h"
#include <libcw/debug.h>

int main(void)
{
#if !CWDEBUG_ALLOC || !CWDEBUG_LOCATION
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
#if CWDEBUG_LOCATION
  Debug( dc::bfd.on() );
#endif

  int* p = new int [100];

  Debug( make_all_allocations_invisible_except(p) );

  Debug( list_allocations_on(libcw_do) );

  delete [] p;

  return 0;
}
