// This test verifies that Location can resolve a return address in A::A() to
// this source file and the actual call-site line, and that mangled_function_name()
// for the caller of test() demangles to A::A().
//
// The expected source line is emitted by the test itself and compared with the
// line number parsed from the location output, so the assertion remains stable
// when this file changes.

#include "cwd_sys.h"
#include "test_support.h"
#include <libcwd/demangle.h>

#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

void test(void* call_site)
{
  libcwd::Location loc(call_site);
  std::string funcname;
  libcwd::demangle_symbol(loc.mangled_function_name(), funcname);
  Dout(dc::notice, "test(): called from " << funcname);
}

class A
{
 public:
  A();
};

A::A()
{
  Dout(dc::notice, "A::A(): called from "
                       << Location((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset));
call_site_test:
  test(&&call_site_test);
}

namespace {

std::vector<std::string> split_lines(std::string const& text)
{
  std::vector<std::string> lines;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) lines.push_back(line);
  return lines;
}

} // namespace

int main()
{
  Debug(main_reached());

  libcwd_ctest::PendingStream captured(libcwd::libcw_do);
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  delete new A;
  int const expected_line = __LINE__ - 1;       // Previous line calls A::A().

  Dout(dc::notice, "expected_line = " << expected_line);

  captured.sync();
  std::cout << "Captured output:\n" << captured.output_str() << std::endl;

  std::vector<std::string> lines = split_lines(captured.output_str());
  std::regex location_re(R"(^NOTICE  : A::A\(\): called from .*location_lookup\.cc:([0-9]+)$)");
  std::regex caller_re(R"(^NOTICE  : test\(\): called from A::A\(\)$)");
  std::regex expected_re(R"(^NOTICE  : expected_line = ([0-9]+)$)");

  bool saw_location = false;
  bool saw_caller = false;
  bool saw_expected = false;
  std::string location_line_number;
  std::string expected_line_number;

  for (std::string const& line : lines)
  {
    std::smatch match;
    if (std::regex_match(line, match, location_re))
    {
      saw_location = true;
      location_line_number = match[1];
    }
    else if (std::regex_match(line, caller_re))
      saw_caller = true;
    else if (std::regex_match(line, match, expected_re))
    {
      saw_expected = true;
      expected_line_number = match[1];
    }
    else
    {
      std::cerr << "Unexpected output line: `" << line << "'\n";
      return EXIT_FAILURE;
    }
  }

  if (!saw_location || !saw_caller || !saw_expected)
  {
    std::cerr << "Missing expected output line(s):"
              << " location=" << saw_location << " caller=" << saw_caller << " expected=" << saw_expected << '\n';
    return EXIT_FAILURE;
  }

  if (location_line_number != expected_line_number)
  {
    std::cerr << "Location line mismatch: Location reported " << location_line_number
              << ", expected line output reported " << expected_line_number << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
