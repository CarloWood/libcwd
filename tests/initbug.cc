#include "sys.h"
#include "debug.h"
#include "initbug_GlobalObject.h"
#include <iostream>

int main()
{
  Debug( main_reached() );
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  Debug( libcw_do.on() );

  std::cout << "Successful\n";

  return 0;
}
