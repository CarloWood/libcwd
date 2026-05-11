#include "sys.h"
#include "test_support.h"

#include <iostream>
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>

namespace libcwd_ctest {

redirect_cerr_ct::redirect_cerr_ct(std::ostream& output) : M_original(std::cerr.rdbuf(output.rdbuf()))
{
}

redirect_cerr_ct::~redirect_cerr_ct()
{
  std::cerr.rdbuf(M_original);
}

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
    if (line != expected[line_number])
    {
      std::cerr << "Output mismatch on line " << line_number << ": expected `"
                << expected[line_number] << "', got `" << line << "'\n";
      return false;
    }
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
