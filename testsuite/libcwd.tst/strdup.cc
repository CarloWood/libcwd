#include "sys.h"
#include <libcwd/debug.h>
#include <cstdlib>
#include <iostream>

MAIN_FUNCTION
{ PREFIX_CODE
  char const* p1 = "Best Story: \"Songmaster\", by Orson Scott Card.";
  char const* p2 = strdup(p1);
  std::cout << p2 << std::endl;
  free((void*)p2);
  EXIT(0);
}
