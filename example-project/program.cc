#include "sys.h"
#include <iostream>
#include "debug.h"

int main(void)
{
  Debug( check_configuration() );
  Debug( dc::custom.on() );
  Debug( libcw_do.on() );

  Dout(dc::custom, "This is debug output, written to a custom channel (see ./debug.h and ./debug.cc)");

  std::cout << "This program works" << std::endl;

  return 0;
}
