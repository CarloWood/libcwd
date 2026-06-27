// Specification test for the cwds API slated to be absorbed into libcwd.
//
// This test is a clean-room consumer of exactly the symbols we intend to move
// out of the cwds submodule into libcwd itself, so that the "Getting Started"
// documentation no longer has to choose between an awkward raw-libcwd setup and
// the modern cwds-based one. It deliberately does NOT use test_support.h's
// namespace helpers: every macro/symbol used here must, after the migration, be
// provided by libcwd through <libcwd/debug.h> (or a header it pulls in).
//
// EXPECTED TO FAIL TO COMPILE until that migration is done. The only reasons
// this file should fail are the still-missing moved symbols listed below; it is
// otherwise intended to be valid C++ that runs as a runtime smoke test once the
// API is in place.
//
// API surface exercised (the agreed move list):
//
//   Namespace macros (user-definable, with libcwd-provided defaults once moved):
//     - NAMESPACE_DEBUG, NAMESPACE_DEBUG_START, NAMESPACE_DEBUG_END,
//       NAMESPACE_CHANNELS
//     - DEBUGCHANNELS is *not* set here; libcwd must derive it from
//       NAMESPACE_DEBUG::NAMESPACE_CHANNELS (as cwds/debug.h does today).
//
//   Provided macros / symbols (libcwd must supply after the move):
//     - NAMESPACE_DEBUG_CHANNELS_START / NAMESPACE_DEBUG_CHANNELS_END
//     - libcwd::thread_init_t (enum with thread_init_default, from_rcfile,
//       copy_from_main, debug_off)
//     - NAMESPACE_DEBUG::init()
//     - NAMESPACE_DEBUG::init_thread(std::string, libcwd::thread_init_t)
//     - DoutEntering(cntrl, ...)
//     - NAMESPACE_DEBUG::Indent (RAII indentation)
//     - NAMESPACE_DEBUG::Mark   (RAII marker; char and char const* ctors, end())

//#include "cwd_sys.h"

// Exercise the custom-debug-namespace path that libraries and applications use.
// After the migration libcwd will default NAMESPACE_DEBUG to `debug` when this
// is left unset; here we set a two-level namespace to also cover that case.
#define NAMESPACE_DEBUG cwds_api_surface::debug
#include <cwds/debug.h>
//#include <libcwd/debug.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#ifndef CWDEBUG
#error CWDEBUG must be defined for this test (it is propagated by the cwd target).
#endif

// Declare an application-owned debug channel through the migrated channel-block
// macros. libcwd must provide NAMESPACE_DEBUG_CHANNELS_START/_END and must have
// brought `Channel` (and the predefined channels) into the derived `dc` scope.
NAMESPACE_DEBUG_CHANNELS_START
Channel api("API");
NAMESPACE_DEBUG_CHANNELS_END

namespace {

// Run inside a worker thread to exercise the per-thread initialization path,
// which is the part of the cwds surface that has no raw-libcwd primitive today.
// Each call uses a different libcwd::thread_init_t enumerator so the enum type
// and all its values are referenced. The body also drives DoutEntering, Indent
// and Mark so the migrated RAII helpers and the entering macro are covered.
void worker(int id, libcwd::thread_init_t mode)
{
  Debug(NAMESPACE_DEBUG::init_thread("worker-" + std::to_string(id), mode));

  DoutEntering(dc::notice, "worker " << id << " entered");

  {
    NAMESPACE_DEBUG::Indent const indent(3);
    NAMESPACE_DEBUG::Mark const marker('|');
    Dout(dc::api, "worker " << id << " doing scoped work");
  }

  // Exercise the char const* constructor of Mark plus explicit end().
  NAMESPACE_DEBUG::Mark m2("\xE2\x96\xB6"); // UTF-8 for BLACK RIGHT-POINTING TRIANGLE.
  Dout(dc::notice, "worker " << id << " after scope");
  m2.end();
}

} // namespace

int main()
{
  // Single-line initialization that replaces the raw main_reached()+read_rcfile()
  // sequence: it runs the configuration-consistency check, reads the rcfile and
  // snapshots channel state for subsequent threads.
  Debug(NAMESPACE_DEBUG::init());

  // Exercise the thread_init_t enumerators directly so the enum is part of the
  // compiled surface, not just a default parameter.
  libcwd::thread_init_t const worker_mode = libcwd::copy_from_main;
  static_cast<void>(libcwd::from_rcfile);
  static_cast<void>(libcwd::debug_off);
  static_cast<void>(libcwd::thread_init_default);

  DoutEntering(dc::notice|dc::api, "main()");

  {
    NAMESPACE_DEBUG::Indent const indent(2);
    NAMESPACE_DEBUG::Mark const marker('M');
    Dout(dc::notice, "Inside an indented, marked scope in main");
  }

  std::thread t1(worker, 1, worker_mode);
  std::thread t2(worker, 2, libcwd::from_rcfile);
  t1.join();
  t2.join();

  std::cout << "cwds_api_surface completed\n";
  return EXIT_SUCCESS;
}
