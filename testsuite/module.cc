// A shared library
#include <libcw/sys.h>
#include <libcw/debug.h>

#include <sys/types.h>
extern "C" void* malloc(size_t);

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
