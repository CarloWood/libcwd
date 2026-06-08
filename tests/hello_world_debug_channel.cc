// Verify the legacy hello_world custom-channel example as a CTest. The test
// defines an application-owned HELLO debug channel using the cwds-style
// namespace helper macros supplied by test_support.h, reads the rcfile override
// supplied by CTest, captures libcw_do output in memory, and checks that the
// channel listing and HELLO message exactly match the expected state. It has no
// filesystem side effects; failures are reported by the shared comparison
// helper on std::cerr.

#include "cwd_sys.h"

// Test using a two-deep namespace for our debug namespace (default is just `debug`).
#define NAMESPACE_DEBUG hello_world_ctest::debug
#include "test_support.h"

#include <cstdlib>
#include <sstream>

// Define the application-owned debug channel used by this test. The label is
// read from the CTest-provided override rcfile, appears in list_channels_on(),
// and is used for the final Dout call. The object is global because libcwd
// channels are initialized and registered before main() reads channel settings.
NAMESPACE_DEBUG_CHANNELS_START
Channel hello("HELLO");
NAMESPACE_DEBUG_CHANNELS_END

int main()
{
  Debug(main_reached());
  Debug(read_rcfile());

  libcwd_ctest::PendingStream captured(libcwd::libcw_do);
  Debug(libcw_do.on());

  Debug(list_channels_on(libcw_do));
  Dout(dc::hello, "Hello World!");

  captured.sync();
  std::cout << "Captured output:\n" << captured.output_str() << std::endl;

  char const* expected[] = {"DEBUG   : Enabled", "ELFUTILS: Enabled",  "HELLO   : Enabled",      "NOTICE  : Disabled",
                            "SYSTEM  : Enabled", "WARNING : Disabled", "HELLO   : Hello World!", nullptr};

  return libcwd_ctest::matches_expected_output(captured.direct_istream(), expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
