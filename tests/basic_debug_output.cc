// Verify the basic libcwd debug-output path. The test enables the default debug
// object and notice channel, redirects std::cerr to an in-memory stream while
// two Dout calls execute, and then compares the captured lines with the output
// expected from cutee/t.basic.h. It exits with a non-zero status on mismatches,
// extra output, or missing output; it has no filesystem side effects.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <sstream>

int main()
{
  Debug(main_reached());

  std::stringstream captured;

  {
    libcwd_ctest::redirect_cerr_ct redirect(captured);
    Debug(libcw_do.on());
    Debug(dc::notice.on());

    Dout(dc::notice, "Basic Test 0.");
    Dout(dc::notice, "Basic Test 1.");
  }

  char const* expected[] = {"NOTICE  : Basic Test 0.", "NOTICE  : Basic Test 1.", nullptr};

  return libcwd_ctest::matches_expected_output(captured, expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
