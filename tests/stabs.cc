#include <libcw/sysd.h>
#include <libcw/debug.h>

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::bfd.on() );
  Debug( dc::malloc.on() );
  char* p = NEW( char [40] );
  return p == 0;
}
