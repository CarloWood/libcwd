#include <libcw/sys.h>
#include <libcw/debug.h>

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
  Debug( dc::bfd.on() );

  int* p = new int [100];

  Debug( list_allocations_on(libcw_do) );

  delete [] p;

  return 0;
}
