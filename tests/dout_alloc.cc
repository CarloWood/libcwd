#include "sys.h"
#include "debug.h"
#include <iostream>

int main(void)
{
  //----------------------------------------------------------------------
  // The following calls will be done in almost every program using libcwd
 
  Debug( check_configuration() );
 
#if CWDEBUG_ALLOC
  // Don't show allocations that are allocated before main()
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif
 
  // Select channels (note that where 'on' is used, 'off' can be specified
  // and vica versa).
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
 
  // Write debug output to cout (the default is cerr)
  Debug( libcw_do.set_ostream(&std::cout) );
 
  // Turn debug object on.  Turning it off can be done recursively.
  // It starts with 'off' depth 1.
  Debug( libcw_do.on() );
 
  // List all debug channels (not very usefull unless you allow to turn
  // channels on and off from the commandline; this is supported in libcw).
  Debug( list_channels_on(libcw_do) );
 
  // End of common block
  //----------------------------------------------------------------------

  int* p;
  Dout(dc::notice, "Allocation inside Dout: " << (p = new int [10]));
  delete [] p;

  return 0;
}
