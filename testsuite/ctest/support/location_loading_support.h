#pragma once

#ifndef LIBCWD_TESTSUITE_CTEST_SUPPORT_LOCATION_LOADING_SUPPORT_H
#define LIBCWD_TESTSUITE_CTEST_SUPPORT_LOCATION_LOADING_SUPPORT_H

#include "test_support.h"

#include <cstdlib>
#include <istream>
#include <memory>
#include <sstream>
#include <string>

namespace libcwd_ctest::location_loading {

using final_output_check_fn = bool (*)(std::istream& captured);

struct capture_state_ct
{
  std::stringstream captured;
  std::unique_ptr<libcwd_ctest::redirect_cerr_ct> redirect;
  int active_libraries = 0;
  final_output_check_fn final_output_check = nullptr;
  bool finalized = false;
  bool passthrough_to_stderr = std::getenv("LIBCWD_PRINT_LOADING") != nullptr;

  void start_capture()
  {
    if (!passthrough_to_stderr && !redirect)
      redirect = std::make_unique<libcwd_ctest::redirect_cerr_ct>(captured);
  }

  void stop_capture()
  {
    redirect.reset();
    captured.clear();
    captured.seekg(0);
  }
};

// Return the process-wide capture state for this fixture.  The object is
// intentionally heap-allocated and leaked so that late shared-object
// destructors can still write and run the final comparison without depending on
// static destruction order across DSOs.
inline capture_state_ct& capture_state()
{
  static capture_state_ct* state = new capture_state_ct;
  return *state;
}

// Initialize libcwd and enable the output path used by the shared-object
// loading fixture.  The function is safe to call from global constructors and
// destructors in multiple shared objects: libcwd::initialize() is idempotent,
// and libcw_do / dc::bfd are only turned on when they are not already active.
// It also starts a std::cerr redirect into an internal stringstream before the
// first log line, so constructors that run before main() are captured.  When
// LIBCWD_PRINT_LOADING is set, capture is disabled instead so libcwd's own BFD
// loading diagnostics remain visible on stderr for manual debugging.  It has
// process-wide side effects on libcwd's debug state and does not report failure;
// fixture code verifies failures through dlopen/dlsym return values and, when
// capturing is enabled, through the final output comparison.
inline void initialize_bfd_logging()
{
  capture_state().start_capture();
  libcwd::initialize();

  LIBCWD_TSD_DECLARATION;
  if (!libcwd::libcw_do.is_on(LIBCWD_TSD))
    libcwd::libcw_do.on();

#if CWDEBUG_LOCATION
  if (!libcwd::channels::dc::bfd.is_on())
    libcwd::channels::dc::bfd.on();
#endif
}

// Register the executable-provided final output check.  This is called from
// main(), after constructors for linked shared objects have already emitted load
// messages but before any final shared-object destructor runs.  The callback is
// invoked once, when the last tracked fixture library destructor reports unload.
inline void register_final_output_check(final_output_check_fn check)
{
  initialize_bfd_logging();
  if (!capture_state().passthrough_to_stderr)
    capture_state().final_output_check = check;
}

// Emit one deterministic dc::bfd message.  This helper is used for both normal
// lifecycle lines and dlopen diagnostics; it captures std::cerr first, has no
// return value, and performs no file:line or symbol lookup.
inline void log_bfd_message([[maybe_unused]] char const* image_name, [[maybe_unused]] std::string const& event)
{
  initialize_bfd_logging();
#if CWDEBUG_LOCATION
  Dout(dc::bfd, image_name << ": " << event);
#endif
}

inline void log_bfd_message(char const* image_name, char const* event)
{
  log_bfd_message(image_name, std::string(event));
}

// Record that a fixture shared object was constructed.  The active-library
// count is incremented before the log line so that a matching destructor can
// detect the final unload.  Inputs are the logical shared-object name used in
// expected output; there is no direct output other than the captured dc::bfd
// line and the process-wide active count side effect.
inline void library_loaded(char const* image_name)
{
  initialize_bfd_logging();
  ++capture_state().active_libraries;
  log_bfd_message(image_name, "loaded");
}

// Record that a fixture shared object is being destroyed.  The unload message
// is logged before decrementing the active-library count so the final line is
// present in the captured stream.  If this was the last tracked library and the
// executable registered a comparison callback, std::cerr is restored and the
// callback is invoked.  A failed comparison aborts the process so CTest observes
// the late destructor-time failure.
inline void library_unloaded(char const* image_name)
{
  initialize_bfd_logging();
  log_bfd_message(image_name, "unloaded");

  capture_state_ct& state = capture_state();
  --state.active_libraries;
  if (state.active_libraries == 0 && state.final_output_check && !state.finalized)
  {
    state.finalized = true;
    state.stop_capture();
    if (!state.final_output_check(state.captured))
      std::abort();
  }
}

} // namespace libcwd_ctest::location_loading

#endif // LIBCWD_TESTSUITE_CTEST_SUPPORT_LOCATION_LOADING_SUPPORT_H
