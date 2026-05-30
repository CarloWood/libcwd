#include "cwd_sys.h"
#include "test_support.h"

#include <istream>
#include <regex>
#include <string>

namespace libcwd_ctest {

namespace {

// Expected lines are literal strings unless they begin with '^' and end in '$';
// those strings are treated as ECMAScript regular expressions and must match the
// whole captured line.
//
// Invalid regular expressions and mismatches are reported to std::cerr with the zero-based output line number.
//
// Returns true iff line matches expected.
bool line_matches_expected(std::string const& line, char const* expected, int line_number)
{
  std::string const expected_line(expected);
  if (expected_line.length() >= 2 && expected_line[0] == '^' && expected_line[expected_line.length() - 1] == '$')
  {
    try
    {
      if (std::regex_match(line, std::regex(expected_line)))
        return true;
    }
    catch (std::regex_error const& error)
    {
      std::cerr << "Invalid expected-output regex on line " << line_number << ": `" << expected_line
                << "': " << error.what() << '\n';
      return false;
    }

    std::cerr << "Output regex mismatch on line " << line_number << ": expected `" << expected_line << "', got `"
              << line << "'\n";
    return false;
  }

  if (line != expected_line)
  {
    std::cerr << "Output mismatch on line " << line_number << ": expected `" << expected_line << "', got `" << line
              << "'\n";
    return false;
  }

  return true;
}

} // namespace

bool matches_expected_output(std::istream& captured, char const* const* expected)
{
  std::string line;
  int line_number = 0;
  while (std::getline(captured, line))
  {
    if (!expected[line_number])
    {
      std::cerr << "Unexpected extra output line: " << line << '\n';
      return false;
    }
    if (!line_matches_expected(line, expected[line_number], line_number))
      return false;
    ++line_number;
  }

  if (expected[line_number])
  {
    std::cerr << "Missing output line: " << expected[line_number] << '\n';
    return false;
  }

  return true;
}

} // namespace libcwd_ctest
