#include "sys.h"
#include "initbug_GlobalObject.h"
#include <libcwd/class_location.h>

#include <iostream>
#include "debug.h"

GlobalObject::GlobalObject()
{
  std::cout << "\nCalling GlobalObject::GlobalObject()\n";
  Dout( dc::notice|continued_cf, "Hello World" );
current_location:
  Dout(dc::always, "We are now at " << location_ct(&&current_location));
  Dout( dc::finish|error_cf, "Hello World" );
}

GlobalObject global_object;

