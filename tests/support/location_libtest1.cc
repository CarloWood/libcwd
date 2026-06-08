#include "cwd_sys.h"
#include "location_loading_support.h"

#include <dlfcn.h>

#ifndef LOCATION_PLUGIN1_PATH
#define LOCATION_PLUGIN1_PATH "location_plugin1.so"
#endif

extern "C" int location_libtest2_touch();

namespace {

void* plugin1_handle;

// Open location_plugin1.so from inside location_libtest1.so.  This function is
// called by the global constructor; it has no inputs, stores the dlopen handle
// for the destructor, and records dlopen errors through the fixture's dc::notice
// output without failing the dynamic loader. RTLD_GLOBAL makes the plugin's
// probe symbol visible to the executable so the CTest program can verify that
// this runtime load happened.
void open_plugin1()
{
  plugin1_handle = dlopen(LOCATION_PLUGIN1_PATH, RTLD_NOW | RTLD_GLOBAL);
  if (!plugin1_handle)
    libcwd_ctest::location_loading::log_notice_message("location_libtest1.so",
                                                       "dlopen(location_plugin1.so) failed: " + std::string(dlerror()));
}

// Logs construction and destruction of location_libtest1.so and performs the
// fixture's library-controlled runtime load of location_plugin1.so.  The object
// depends on location_libtest2.so through location_libtest1_touch(), so the
// loader should bring libtest2 in automatically as libtest1's private needed
// library before this constructor runs.
class location_libtest1_LifeCycle
{
 public:
  location_libtest1_LifeCycle()
  {
    libcwd_ctest::location_loading::library_loaded("location_libtest1.so");
    open_plugin1();
  }

  ~location_libtest1_LifeCycle()
  {
    libcwd_ctest::location_loading::library_unloaded("location_libtest1.so");
    if (plugin1_handle)
    {
      dlclose(plugin1_handle);
      plugin1_handle = nullptr;
    }
  }
};

location_libtest1_LifeCycle location_libtest1_lifecycle;

} // namespace

// Exported probe called by the executable.  Calling location_libtest2_touch()
// keeps location_libtest2.so as a real dependency of this shared object and
// returns a deterministic value that proves the dependency call path works.
extern "C" int location_libtest1_touch()
{
  // Return 12.
  return 10 + location_libtest2_touch();
}
