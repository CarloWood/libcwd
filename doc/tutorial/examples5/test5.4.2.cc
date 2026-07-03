#include "debug.h"

int main()
{
  Debug(main_reached());
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  Dout(dc::notice|nonewline_cf, "Hello, ");
  Dout(dc::notice|noprefix_cf, "World");
}
