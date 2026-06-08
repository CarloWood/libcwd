// Port of the historical tests/lines.cc regression test.
//
// This test verifies that Location resolves a return address to the physical
// call-site line and honors #line directives by reporting the logical source file
// and line emitted into the DWARF line table.  It performs direct assertions
// instead of matching debug output, which makes it suitable for CTest.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

bool expect_location(char const* scenario, libcwd::Location const& location, char const* expected_file,
                     unsigned int expected_line)
{
  if (!location.is_known())
  {
    std::cerr << scenario << ": location is unknown\n";
    return false;
  }

  if (location.file() != expected_file || location.line() != expected_line)
  {
    std::cerr << scenario << ": got " << location.file() << ':' << location.line() << ", expected " << expected_file
              << ':' << expected_line << '\n';
    return false;
  }

  return true;
}

[[gnu::noinline]] libcwd::Location caller_location()
{
  return libcwd::Location((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
}

bool probe_physical_line()
{
  return expect_location("physical line", caller_location(), "location_line_directives.cc", __LINE__);
}

bool probe_line_directives()
{
  bool ok = true;
#line 200 "foo.c"
  ok = expect_location("#line foo.c", caller_location(), "foo.c", __LINE__) && ok;
#line 300 "bar.c"
  ok = expect_location("#line bar.c", caller_location(), "bar.c", __LINE__) && ok;
#line 52 "location_line_directives.cc"
  return ok;
}

} // namespace

int main()
{
  Debug(main_reached());
  return probe_physical_line() && probe_line_directives() ? EXIT_SUCCESS : EXIT_FAILURE;
}
