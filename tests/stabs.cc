#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>
#include <libcwd/debug.h>

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::bfd.on() );
  Debug( dc::malloc.on() );
  char* p = NEW( char [40] );
  return p == 0;
}
