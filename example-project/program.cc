#include "sys.h"
#include <iostream.h>
#include "debug.h"

int main(void)
{
  Debug( check_configuration() );
  Debug( dc::custom.on() );
  Debug( libcw_do.on() );

  Dout(dc::custom, "This is debug output, written to a custom channel (see ./debug.h and ./debug.cc)");

  cout << "This program works" << endl;

  return 0;
}
