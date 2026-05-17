#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>
#include <libcwd/debug.h>

void print_line()
{
  libcwd::location_ct loc((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
  Dout(dc::notice, loc);
}

int main()
{
  Debug( libcw_do.on() );
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );

  int i = 0; print_line();
#line __LINE__ "foo.c"          // __LINE__ = 20
  i++; print_line();            // Therefore this has location foo.c:20
#line __LINE__ "bar.c"          // and __LINE__ = 21 here.
  print_line(); return i;       // Therefore this has location bar.c:21
}
