#include "sys.h"
#include "location_loading_support.h"

namespace {

// Logs construction and destruction of location_plugin2.so.  The test
// executable dlopen-s this plugin directly to cover the application-controlled
// dynamic-loading path separately from plugin1's library-controlled path.
class location_plugin2_lifecycle_ct
{
 public:
  location_plugin2_lifecycle_ct()
  {
    libcwd_ctest::location_loading::library_loaded("location_plugin2.so");
  }

  ~location_plugin2_lifecycle_ct()
  {
    libcwd_ctest::location_loading::library_unloaded("location_plugin2.so");
  }
};

location_plugin2_lifecycle_ct location_plugin2_lifecycle;

} // namespace

// Exported probe for the executable's direct dlopen path.  It returns a fixed
// value and does not inspect DWARF, symbols, or source locations.
extern "C" int location_plugin2_touch()
{
  return 202;
}
