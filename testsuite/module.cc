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

void* realloc1000_no_AllocTag(void* p)
{
  return realloc(p, 1000);
}

void* realloc1000_with_AllocTag(void* p)
{
  void* res = realloc(p, 1000);
  return res;
}

void* new1000(size_t s)
{
  char* res = new char[s];
  return res;
}
