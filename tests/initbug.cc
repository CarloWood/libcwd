#include "sys.h"
#include "debug.h"
#include "initbug_GlobalObject.h"
#include <iostream>

int main(void)
{
  Debug( check_configuration() );
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  Debug( libcw_do.on() );

  std::cout << "Successful\n";

  return 0;
}
