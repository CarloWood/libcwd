// Port of the historical testsuite/libcwd.tst/location.cc regression test.
//
// This test verifies Location lookup for return addresses captured from a
// small noinline call chain without relying on platform-specific recursive
// __builtin_return_address depths.  Several noinline functions capture their
// immediate caller address and verify that libcwd resolves it to the expected
// source line.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <iostream>

namespace {

bool expect_call_site(char const* scenario, void const* return_address, unsigned int expected_line)
{
  libcwd::Location location((char*)return_address + libcwd::builtin_return_address_offset);
  if (!location.is_known())
  {
    std::cerr << scenario << ": location is unknown\n";
    return false;
  }

  if (location.file() != "location_return_addresses.cc" || location.line() != expected_line)
  {
    std::cerr << scenario << ": got " << location.file() << ':' << location.line()
              << ", expected location_return_addresses.cc:" << expected_line << '\n';
    return false;
  }

  return true;
}

[[gnu::noinline]] bool leaf(char const* scenario, unsigned int expected_line)
{
  return expect_call_site(scenario, __builtin_return_address(0), expected_line);
}

[[gnu::noinline]] bool level3()
{
  return leaf("level3 -> leaf", __LINE__);
}

[[gnu::noinline]] bool level2()
{
  bool const ok = level3();
  return leaf("level2 -> leaf", __LINE__) && ok;
}

[[gnu::noinline]] bool level1()
{
  bool const ok = level2();
  return leaf("level1 -> leaf", __LINE__) && ok;
}

} // namespace

int main()
{
  Debug(main_reached());
  return level1() ? EXIT_SUCCESS : EXIT_FAILURE;
}
