#ifndef LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H
#define LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H

#include <iosfwd>

namespace libcwd_ctest {

// Redirects std::cerr to the stream passed to the constructor and restores the
// original stream buffer in the destructor. The object has no ownership of the
// destination stream; callers must keep that stream alive until destruction.
// Copying is disabled to ensure each redirect restores std::cerr exactly once.
class redirect_cerr_ct {
 private:
  std::streambuf* M_original;

 public:
  explicit redirect_cerr_ct(std::ostream& output);
  redirect_cerr_ct(redirect_cerr_ct const&) = delete;
  redirect_cerr_ct& operator=(redirect_cerr_ct const&) = delete;
  ~redirect_cerr_ct();
};

// Compare captured line-oriented output with a null-terminated array of expected
// lines. The function consumes all complete lines from captured, reports the
// first missing, extra, or mismatching line to std::cerr, and returns false on
// failure. It returns true only when the captured output exactly matches the
// expected lines; a final trailing newline is accepted because std::getline
// discards line terminators.
bool matches_expected_output(std::istream& captured, char const* const* expected);

} // namespace libcwd_ctest

#endif // LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H
