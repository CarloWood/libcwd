#include <libcw/sysd.h>
#include <libcw/debug.h>

void print_line(void)
{
  libcw::debug::location_ct loc((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset);
  Dout(dc::notice, loc);
}

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );

    int i = 0; print_line();
#line __LINE__ "foo.c"
    i++; print_line();
#line __LINE__ "bar.c"
    print_line(); return i;
}

