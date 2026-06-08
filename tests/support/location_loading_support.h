#pragma once

#ifndef LIBCWD_TESTSUITE_CTEST_SUPPORT_LOCATION_LOADING_SUPPORT_H
#define LIBCWD_TESTSUITE_CTEST_SUPPORT_LOCATION_LOADING_SUPPORT_H

#include "test_support.h"

#include <istream>
#include <sstream>
#include <string>
#include <vector>

namespace libcwd_ctest::location_loading {

using final_output_check_fn = bool (*)(std::istream& captured);

struct CaptureState
{
  std::stringstream captured;
  int active_libraries = 0;
  final_output_check_fn final_output_check = nullptr;
  bool finalized = false;
  bool logging_initialized = false;
  std::vector<std::string> pending_messages;

  void stop_capture()
  {
    captured.clear();
    captured.seekg(0);
  }
};

// Return the process-wide capture state for this fixture.  The object is
// intentionally heap-allocated and leaked so that late shared-object
// destructors can still write and run the final comparison without depending on
// static destruction order across DSOs.
inline CaptureState& capture_state()
{
  static CaptureState* state = new CaptureState;
  return *state;
}

// Initialize libcwd's ordinary NOTICE output path after main() has started.
//
// This fixture is not testing ELF/DWARF diagnostics. Constructors that run
// before main() only record their lifecycle events in memory; main() calls this
// function after Debug(main_reached()), at which point the captured stream,
// libcw_do and dc::notice are set up like the other CTest programs. Destructors
// that run after main() can keep using the leaked capture state.
inline void initialize_notice_logging()
{
  CaptureState& state = capture_state();
  if (state.logging_initialized)
    return;

  Debug(libcwd::libcw_do.set_ostream(&state.captured));
  Debug(libcwd::libcw_do.on());
  Debug(dc::notice.on());
  state.logging_initialized = true;

  for (std::string const& message : state.pending_messages)
    Dout(dc::notice, message);
  state.pending_messages.clear();
}

// Register the executable-provided final output check.  This is called from
// main(), after constructors for linked shared objects have already emitted load
// messages but before any final shared-object destructor runs.  The callback is
// invoked once, when the last tracked fixture library destructor reports unload.
inline void register_final_output_check(final_output_check_fn check)
{
  initialize_notice_logging();
  capture_state().final_output_check = check;
}

// Emit one deterministic dc::notice message. This helper is used for both normal
// lifecycle lines and dlopen diagnostics; it has no return value and performs no
// file:line or symbol lookup.
inline void log_notice_message(char const* image_name, std::string const& event)
{
  CaptureState& state = capture_state();
  std::string const message = std::string(image_name) + ": " + event;
  if (state.logging_initialized)
    Dout(dc::notice, message);
  else
    state.pending_messages.push_back(message);
}

inline void log_notice_message(char const* image_name, char const* event)
{
  log_notice_message(image_name, std::string(event));
}

// Record that a fixture shared object was constructed.  The active-library
// count is incremented before the log line so that a matching destructor can
// detect the final unload.  Inputs are the logical shared-object name used in
// expected output; there is no direct output before main() other than recording
// the pending dc::notice line and the process-wide active count side effect.
inline void library_loaded(char const* image_name)
{
  CaptureState& state = capture_state();
  ++state.active_libraries;
  log_notice_message(image_name, "loaded");
}

// Record that a fixture shared object is being destroyed.  The unload message
// is logged before decrementing the active-library count so the final line is
// present in the captured stream. If this was the last tracked library and the
// executable registered a comparison callback, the captured stream is rewound and
// the callback is invoked. A failed comparison aborts the process so CTest
// observes the late destructor-time failure.
inline void library_unloaded(char const* image_name)
{
  initialize_notice_logging();
  log_notice_message(image_name, "unloaded");

  CaptureState& state = capture_state();
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
