#pragma once

#ifndef LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H
#define LIBCWD_TESTSUITE_CTEST_SUPPORT_TEST_SUPPORT_H

//=============================================================================
// Macros that are normally a part of cwds.

// Test sources include sys.h first, optionally define namespace selection macros
// such as NAMESPACE_DEBUG, and then include this header.
//
// This configures libcwd's DEBUGCHANNELS before including <libcwd/debug.h>,
// declares the application debug-channel namespace, and exposes
// NAMESPACE_DEBUG_CHANNELS_START/END for defining custom channels.

#ifndef NAMESPACE_DEBUG
#define NAMESPACE_DEBUG debug
#endif
#ifndef NAMESPACE_DEBUG_START
#define NAMESPACE_DEBUG_START namespace NAMESPACE_DEBUG {
#define NAMESPACE_DEBUG_END }
#endif

#ifndef NAMESPACE_CHANNELS
#define NAMESPACE_CHANNELS channels
#endif

#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::NAMESPACE_DEBUG::NAMESPACE_CHANNELS
#endif

#include <libcwd/debug.h>

#define NAMESPACE_DEBUG_CHANNELS_START namespace NAMESPACE_DEBUG::NAMESPACE_CHANNELS::dc {
#define NAMESPACE_DEBUG_CHANNELS_END }

NAMESPACE_DEBUG_CHANNELS_START
using namespace libcwd::channels::dc;
using libcwd::channel_ct;
NAMESPACE_DEBUG_CHANNELS_END

// End of cwds macros.
//=============================================================================

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
