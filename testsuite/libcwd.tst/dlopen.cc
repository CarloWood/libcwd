#include "../../sys.h"
#include <libcw/debug.h>
#include <dlfcn.h>

typedef void* (*f_type)(bool);
f_type f;

int main(void)
{
  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );

  void* handle = dlopen("./module.so", RTLD_NOW|RTLD_GLOBAL);

  if (!handle)
  {
    char* error_str = dlerror();
    DoutFatal(dc::fatal, "Failed to load \"./module.so\": " << error_str);
  }

#if __GXX_ABI_VERSION == 0
  char const* sym = "global_test_symbol__Fb";
#else
  char const* sym = "_Z18global_test_symbolb";
#endif
  f = (f_type)dlsym(handle, sym);

  if (!f)
  {
    char* error_str = dlerror();
    DoutFatal(dc::fatal, "Failed find function \"" << sym << "\": " << error_str);
  }

  (*f)(false);
  (*f)(true);

  Debug( list_allocations_on(libcw_do) );
  Dout(dc::notice, "Finished");

  exit(0);
}
