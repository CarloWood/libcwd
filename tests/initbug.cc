#include <libcw/sys.h>
#include <libcw/debug.h>
#include "initbug_GlobalObject.h"

int main(void)
{
  Debug( check_configuration() );
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  Debug( libcw_do.on() );

  cout << "Successful\n";

  return 0;
}
