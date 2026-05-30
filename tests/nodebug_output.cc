// Verify the libcwd.nodebug test output under CTest.
//
// This is a direct CTest form of testsuite/libcwd.nodebug/nodebug.cc: it turns
// on the default debug object and ELFUTILS channel, resolves a local label with
// location_ct, and compares the resulting printed output line-by-line.

#include "cwd_sys.h"
#include "pending_stream.h"
#include "test_support.h"

#include <cstdlib>
#include <sstream>
#include <string>

int main()
{
  Debug(main_reached());

#if CWDEBUG_LOCATION
  // Prime the object/symbol cache before enabling the ELFUTILS channel; otherwise the
  // cache loading trace would become part of the output that this test compares.
  libcwd::location_ct warmup(&&warmup_location);
warmup_location:
#endif

  libcwd_ctest::PendingStream captured;
  Debug(libcw_do.set_ostream(&captured));
  Debug(libcw_do.on());
#if CWDEBUG_LOCATION
  Debug(dc::elfutils.on());
  Debug(location_format(show_objectfile | show_function));

  libcwd::location_ct const loc(&&current_location);
current_location:
  Dout(dc::elfutils, "This is printed from " << loc);
#endif

  captured.rdbuf()->pubsync();
  std::cout << "Captured output:\n" << captured.output_str() << std::endl;

#if CWDEBUG_LOCATION
  std::string const expected_location = "ELFUTILS: This is printed from nodebug_output:main";
#endif
  char const* expected[] = {
#if CWDEBUG_LOCATION
      expected_location.c_str(),
#endif
      nullptr};

  return libcwd_ctest::matches_expected_output(captured.input(), expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
