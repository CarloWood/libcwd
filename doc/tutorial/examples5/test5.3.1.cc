#include "debug.h"    // See § 5.2

int main()
{
  Debug(main_reached());
  // Start with everything turned on:
  Debug(libcw_do.on());
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on());

  Dout(dc::elephant|dc::owl, "The elephant is called Dumbo, the owl is called Einstein.");
}
