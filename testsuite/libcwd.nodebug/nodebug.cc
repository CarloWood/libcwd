#include <libcw/sys.h>
#include <libcw/debug.h>

int main(void)
{
#if !defined(DEBUGMALLOC) || !defined(DEBUGUSEBFD)
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
#ifdef DEBUGUSEBFD
  Debug( dc::bfd.on() );
#endif

  int* p = new int [100];

  Debug( list_allocations_on(libcw_do) );

  delete [] p;

  return 0;
}
