#include "sys.h"
#include "debug.h"
#include <iostream>
#include <errno.h>
#include "initbug_GlobalObject.h"

GlobalObject::GlobalObject(void)
{
  std::cout << "\nCalling GlobalObject::GlobalObject(void)\n";
  Dout( dc::notice|continued_cf, "Hello World" );
  new int [20];
  Dout( dc::finish|error_cf, "Hello World" );
}

GlobalObject global_object;

