// A shared library
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcw/sysd.h>
#include "alloctag_debug.h"

static void* static_test_symbol(void)
{
  void* ptr = malloc(300);
  AllocTag(ptr, "Allocated inside static_test_symbol");
  return ptr;
}

void* global_test_symbol(bool do_static)
{
  if (do_static)
    return static_test_symbol();
  void* ptr = malloc(310);
  AllocTag(ptr, "Allocated inside global_test_symbol");
  return ptr;
}

void* realloc1000_no_AllocTag(void* p)
{
  return realloc(p, 1000);
}

void* realloc1000_with_AllocTag(void* p)
{
  void* res = realloc(p, 1000);
  AllocTag(res, "realloc1000_with_AllocTag");
  return res;
}

void* new1000(size_t s)
{
  char* res = new char[s];
  AllocTag(res, "new1000");
  return res;
}

