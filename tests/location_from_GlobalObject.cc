#include "cwd_sys.h"
#include "debug.h"
#include <libcwd/class_location.h>
#include <iostream>

class GlobalObject {
public:
  GlobalObject();
};

GlobalObject::GlobalObject()
{
  LIBCWD_TSD_DECLARATION;
  Debug(
    if (!dc::notice.is_on())
      dc::notice.on();
    if (!libcw_do.is_on(LIBCWD_TSD))
      libcw_do.on()
  );

  std::cout << "Calling GlobalObject::GlobalObject()\n";
  Dout( dc::notice|continued_cf, "Hello World" );
current_location:
  Dout(dc::always, "We are now at " << location_ct(&&current_location));
  Dout( dc::finish|error_cf, "Hello World" );
}

GlobalObject global_object;
int main()
{
  Debug( main_reached() );
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  Debug( libcw_do.on() );

  std::cout << "Successful\n";

  return 0;
}
