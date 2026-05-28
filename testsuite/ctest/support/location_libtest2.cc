#include "cwd_sys.h"
#include "location_loading_support.h"

namespace {

// Logs construction and destruction of location_libtest2.so.  The constructor
// has no inputs or outputs; its side effect is to initialize libcwd early and
// write a dc::elfutils message proving that a DT_NEEDED dependency was loaded before
// the executable reaches main().  The destructor emits the matching unload
// message and assumes libcwd's global debug state is still usable during DSO
// teardown.
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
