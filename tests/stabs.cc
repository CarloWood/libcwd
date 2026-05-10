#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>
#include <libcwd/debug.h>

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );

  // Print the source location for an address inside main. This keeps the test
  // focused on location lookup instead of depending on the removed malloc debug
  // channel to print the allocation site of the following NEW expression.
allocation_location:
  Dout(dc::notice, "allocation location: " << location_ct(&&allocation_location));
  char* p = new char [40];
  return p == 0;
}
