#include <libcw/sys.h>
#include <iostream>
#include <errno.h>
#include <libcw/debug.h>
#include "initbug_GlobalObject.h"

GlobalObject::GlobalObject(void)
{
  cout << "\nCalling GlobalObject::GlobalObject(void)\n";
  Dout( dc::notice|continued_cf, "Hello World" );
  new int [20];
  Dout( dc::finish|error_cf, "Hello World" );
}

GlobalObject global_object;
