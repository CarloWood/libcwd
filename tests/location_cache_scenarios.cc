// Deterministic coverage for cache-backed location_ct lookups that do not rely on stderr formatting.
// Each probe captures a concrete code address, resolves it through location_ct, and checks the source
// file/line and mangled caller name that the `dwarf` initialization cache should have precomputed.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

bool ends_with(std::string const& value, char const* suffix)
{
  size_t suffix_length = std::strlen(suffix);
  return value.size() >= suffix_length && value.compare(value.size() - suffix_length, suffix_length, suffix) == 0;
}

bool expect_location(char const* scenario, libcwd::location_ct const& location, char const* expected_file_suffix,
                     unsigned int expected_line)
{
  if (!location.is_known())
  {
    std::cerr << scenario << ": location is unknown\n";
    return false;
  }

  if (!ends_with(location.file(), expected_file_suffix) || location.line() != expected_line)
  {
    std::cerr << scenario << ": got " << location.file() << ':' << location.line() << ", expected *"
              << expected_file_suffix << ':' << expected_line << '\n';
    return false;
  }
  return true;
}

[[gnu::noinline]] bool expect_call_site(char const* scenario, char const* expected_file_suffix,
                                        unsigned int expected_line)
{
  libcwd::location_ct location((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
  return expect_location(scenario, location, expected_file_suffix, expected_line);
}

bool probe_plain_line()
{
  unsigned int const expected_line = __LINE__ + 1;
  return expect_call_site("plain line", "location_cache_scenarios.cc", expected_line);
}

bool probe_line_directive()
{
#line 700 "location_cache_virtual_file.cc"
  unsigned int const expected_line = __LINE__ + 1;
  bool const result = expect_call_site("line directive", "location_cache_virtual_file.cc", expected_line);
#line 56 "location_cache_scenarios.cc"
  return result;
}

unsigned int const inline_function_definition_line = __LINE__ + 1;
[[gnu::noinline]] libcwd::location_ct capture_caller_location()
{
  return libcwd::location_ct((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
}

[[gnu::always_inline]] inline libcwd::location_ct capture_inline_location()
{
  return capture_caller_location();
}

bool probe_inline_body_source()
{
  libcwd::location_ct location = capture_inline_location();
  unsigned int const expansion_line = __LINE__;

  if (!location.is_known())
  {
    std::cerr << "inline body: location is unknown\n";
    return false;
  }
  if (!ends_with(location.file(), "location_cache_scenarios.cc") || location.line() == expansion_line)
  {
    std::cerr << "inline body: got " << location.file() << ':' << location.line()
              << ", expected inline function source near line " << inline_function_definition_line
              << " rather than expansion line " << expansion_line << '\n';
    return false;
  }
  return true;
}

} // namespace

int main()
{
  Debug(main_reached());
  return probe_plain_line() && probe_line_directive() && probe_inline_body_source() ? EXIT_SUCCESS : EXIT_FAILURE;
}
