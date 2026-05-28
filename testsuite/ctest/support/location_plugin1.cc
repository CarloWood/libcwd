#include "cwd_sys.h"
#include "location_loading_support.h"

namespace {

// Logs construction and destruction of location_plugin1.so.  This plugin is
// dlopen-ed by location_libtest1.so at runtime, so these messages exercise
// libcwd initialization from a DSO that is not a static link dependency of the
// test executable.  The destructor mirrors the constructor and performs only a
// dc::elfutils logging side effect.
class location_plugin1_lifecycle_ct
{
 public:
  location_plugin1_lifecycle_ct()
  {
    libcwd_ctest::location_loading::library_loaded("location_plugin1.so");
  }

  ~location_plugin1_lifecycle_ct()
  {
    libcwd_ctest::location_loading::library_unloaded("location_plugin1.so");
  }
};

location_plugin1_lifecycle_ct location_plugin1_lifecycle;

} // namespace

// Exported probe used by the test after RTLD_GLOBAL loading from libtest1.  The
// return value is intentionally fixed and has no side effects, allowing the
// executable to verify that plugin1 was loaded without involving location data.
extern "C" int location_plugin1_touch()
{
  return 101;
}
