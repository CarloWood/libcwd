#include <libcw/sys.h>
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

  void* handle = dlopen("module.so", RTLD_NOW|RTLD_GLOBAL);

  if (!handle)
  {
    char* error_str = dlerror();
    DoutFatal(dc::fatal, "Failed to load \"module.so\": " << error_str);
  }

  f = (f_type)dlsym(handle, "_Z18global_test_symbolb");

  if (!f)
  {
    char* error_str = dlerror();
    DoutFatal(dc::fatal, "Failed find function \"_Z18global_test_symbolb\": " << error_str);
  }

  (*f)(false);
  (*f)(true);

  Debug( list_allocations_on(libcw_do) );
  Dout(dc::notice, "Finished");

  exit(0);
}
