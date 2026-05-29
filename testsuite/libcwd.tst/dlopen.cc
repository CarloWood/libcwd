#include "sys.h"
#include <libcwd/debug.h>
#include <dlfcn.h>
#include <cerrno>

typedef void* (*f_type)(bool);
f_type f;

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( main_reached() );


  Debug( libcw_do.on() );
#if CWDEBUG_LOCATION
  Debug( dc::elfutils.on() );
#endif
  Debug( dc::notice.on() );

#ifdef STATIC
  Dout(dc::notice, "You cannot use dlopen in a statically linked application.");
#else // !STATIC

  void* handle;
  do
  {
    char const* module_name = "./libmodule_r.so";
    handle = dlopen(module_name, RTLD_NOW|RTLD_GLOBAL);

    if (!handle)
    {
      if (errno != EAGAIN)
      {
	char const* error_str = dlerror();
	DoutFatal(dc::fatal, "Failed to load \"" << module_name << "\": " << error_str);
      }
    }
  }
  while (!handle);

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
  char const* sym = "global_test_symbol__Fb";
#else
  char const* sym = "_Z18global_test_symbolb";
#endif
  f = (f_type)dlsym(handle, sym);

  if (!f)
  {
    char const* error_str = dlerror();
    DoutFatal(dc::fatal, "Failed find function \"" << sym << "\": " << error_str);
  }

  void* ptr1 = (*f)(false);
  void* ptr2 = (*f)(true);

  free(ptr1);
  free(ptr2);
  dlclose(handle);

#endif // !STATIC

  Dout(dc::notice, "Finished");

  EXIT(0);
}
