// A shared library
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "libcwd/config.h"
#ifndef STATIC
#include <libcwd/sys.h>
#endif
#include <cstdlib>

static void* static_test_symbol()
{
  void* ptr = malloc(300);
  return ptr;
}

void* global_test_symbol(bool do_static)
{
  if (do_static)
    return static_test_symbol();
  void* ptr = malloc(310);
  return ptr;
}
