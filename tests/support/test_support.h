#pragma once

#ifndef LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H
#define LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H

#include <iosfwd>
#include <iostream>

namespace libcwd_ctest {

// Redirects std::cerr to the stream passed to the constructor and restores the
// original stream buffer in the destructor. The object has no ownership of the
// destination stream; callers must keep that stream alive until destruction.
// Copying is disabled to ensure each redirect restores std::cerr exactly once.
class RedirectCerr
{
 private:
  std::streambuf* original_;

 public:
  explicit RedirectCerr(std::ostream& output) : original_(std::cerr.rdbuf(output.rdbuf())) { }
  RedirectCerr(RedirectCerr const&) = delete;
  RedirectCerr& operator=(RedirectCerr const&) = delete;
  ~RedirectCerr() { std::cerr.rdbuf(original_); }
};

// Compare captured line-oriented output with a null-terminated array of expected lines.
// The function consumes all complete lines from captured, reports the first missing, extra,
// or mismatching line to std::cerr, and returns false on failure.
//
// The function returns true only when all captured output matches the expected lines;
// a final trailing newline is accepted because std::getline discards line terminators.
bool matches_expected_output(std::istream& captured, char const* const* expected);

} // namespace libcwd_ctest

#include "PendingStream.h"

#endif // LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H
