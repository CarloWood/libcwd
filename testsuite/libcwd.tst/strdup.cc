#include "sys.h"
#include <libcw/debug.h>
#include <cstdlib>
#include <iostream>

int main(void)
{
  char const* p1 = "Best Story: \"Songmaster\", by Orson Scott Card.";
  char const* p2 = strdup(p1);
  std::cout << p2 << std::endl;
  free((void*)p2);
  return 0;
}
