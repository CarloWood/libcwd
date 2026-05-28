#include "cwd_sys.h"
#include "location_loading_support.h"

namespace {

// Logs construction and destruction of location_libtest2.so.  The constructor
// has no inputs or outputs; its side effect is to record that a DT_NEEDED
// dependency was loaded before the executable reaches main(). The destructor
// emits the matching unload message after main has initialized dc::notice output.
class location_libtest2_lifecycle_ct
{
 public:
  location_libtest2_lifecycle_ct()
  {
    libcwd_ctest::location_loading::library_loaded("location_libtest2.so");
  }

  ~location_libtest2_lifecycle_ct()
  {
    libcwd_ctest::location_loading::library_unloaded("location_libtest2.so");
  }
};

location_libtest2_lifecycle_ct location_libtest2_lifecycle;

} // namespace

// Export a tiny symbol that location_libtest1.so calls.  This keeps
// location_libtest2.so as a real private link dependency of libtest1 and gives
// the executable an observable way to confirm that calls cross the dependency.
extern "C" int location_libtest2_touch()
{
  return 2;
}
