// A shared library
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef STATIC
#include <libcwd/sys.h>
#include "alloctag_debug.h"
#endif
#include <cstdlib>

static void* static_test_symbol(void)
{
  void* ptr = malloc(300);
#ifndef STATIC
  AllocTag(ptr, "Allocated inside static_test_symbol");
#endif
  return ptr;
}

void* global_test_symbol(bool do_static)
{
  if (do_static)
    return static_test_symbol();
  void* ptr = malloc(310);
#ifndef STATIC
  AllocTag(ptr, "Allocated inside global_test_symbol");
#endif
  return ptr;
}

void* realloc1000_no_AllocTag(void* p)
{
  return realloc(p, 1000);
}

void* realloc1000_with_AllocTag(void* p)
{
  void* res = realloc(p, 1000);
#ifndef STATIC
  AllocTag(res, "realloc1000_with_AllocTag");
#endif
  return res;
}

void* new1000(size_t s)
{
  char* res = new char[s];
#ifndef STATIC
  AllocTag(res, "new1000");
#endif
  return res;
}

