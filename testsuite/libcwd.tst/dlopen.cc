#include "sys.h"
#include <libcw/debug.h>
#include <dlfcn.h>
#include <cerrno>

typedef void* (*f_type)(bool);
f_type f;

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );

#if CWDEBUG_ALLOC && !defined(THREADTEST)
  new int;							// Make sure initialization of libcwd is done.
  libcw::debug::make_all_allocations_invisible_except(NULL);	// Don't show allocations that are done as part of initialization.
#endif

  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );

  void* handle;
  
  do
  {
#ifdef THREADTEST
    char const* module_name = "./module_r.so";
#else
    char const* module_name = "./module.so";
#endif
    handle = dlopen(module_name, RTLD_NOW|RTLD_GLOBAL);

    if (!handle)
    {
      if (errno != EINTR && errno != EAGAIN)
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

  (*f)(false);
  (*f)(true);

  Debug( list_allocations_on(libcw_do) );
  Dout(dc::notice, "Finished");

  Debug( dc::malloc.off() );

  EXIT(0);
}
