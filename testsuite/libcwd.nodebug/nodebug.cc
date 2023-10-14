#include "sys.h"
#include <libcwd/debug.h>

#ifndef CWDEBUG_ALLOC
#error "config.h isn't included"
#endif

int main()
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

#if CWDEBUG_ALLOC
  Debug( make_all_allocations_invisible_except(p) );

  Debug( list_allocations_on(libcw_do) );
#endif

  delete [] p;

  return 0;
}
