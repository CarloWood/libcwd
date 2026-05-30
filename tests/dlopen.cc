// Verify source-location lookup for code in a dlopen-loaded shared module.
//
// This is a CTest-backed replacement for the useful part of the historical
// testsuite/libcwd.tst/dlopen.cc test. The fixture loads a companion module,
// resolves the same `_Z18global_test_symbolb' C++ symbol, calls it with both
// boolean values, and checks the dc::notice output for the expected module file,
// function names, and self-reported line numbers without relying on any
// removed malloc-debugging output or unstable hexadecimal addresses.

#include "cwd_sys.h"
#include "test_support.h"
#include "pending_stream.h"

#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <sstream>

#ifndef DLOPEN_MODULE_PATH
#define DLOPEN_MODULE_PATH "dlopen_module.so"
#endif

namespace {

using module_function_type = bool (*)(bool);

// Resolve global_test_symbol(bool) from handle and store it in function.
//
// The symbol name intentionally matches the one used by the legacy DejaGnu test.
// The function returns false after printing a dlerror() diagnostic when dlsym
// fails, and leaves function unchanged in that case.
bool resolve_global_test_symbol(void* handle, module_function_type& function)
{
  char const* sym = "_Z18global_test_symbolb";
  dlerror();
  void* symbol = dlsym(handle, sym);
  char const* error = dlerror();
  if (error)
  {
    std::cerr << "dlsym(" << sym << ") failed: " << error << '\n';
    return false;
  }

  function = reinterpret_cast<module_function_type>(symbol);
  return true;
}

// Check the complete notice output from the two module calls.
//
// Each regex verifies the logical branch name, the module object filename, the
// expected function name, and the source filename. The back-reference
// checks that the line printed by location_ct is the same line number that the
// module passed as its expectation.
bool compare_dlopen_output(std::istream& captured)
{
  char const* expected[] = {
    "^NOTICE  : global_test_symbol\\(false\\): .*dlopen_module\\.so:_Z18global_test_symbolb:dlopen_module\\.cc:([0-9]+) \\(expected line: \\1\\)$",
    "^NOTICE  : static_test_symbol: .*dlopen_module\\.so:static_test_symbol:dlopen_module\\.cc:([0-9]+) \\(expected line: \\1\\)$",
    nullptr
  };

  return libcwd_ctest::matches_expected_output(captured, expected);
}

} // namespace

int main()
{
  Debug(main_reached());

  libcwd_ctest::PendingStream captured;
  Debug(libcw_do.set_ostream(&captured));
  Debug(libcw_do.on());
  Debug(dc::notice.on());
  Debug(location_format(show_objectfile|show_function));

  void* handle = dlopen(DLOPEN_MODULE_PATH, RTLD_NOW | RTLD_GLOBAL);
  if (!handle)
  {
    std::cerr << "dlopen(" << DLOPEN_MODULE_PATH << ") failed: " << dlerror() << '\n';
    return EXIT_FAILURE;
  }

  module_function_type global_test_symbol = nullptr;
  bool success = resolve_global_test_symbol(handle, global_test_symbol);
  if (success)
  {
    success = global_test_symbol(false) && success;
    success = global_test_symbol(true) && success;
  }

  if (dlclose(handle) != 0)
  {
    std::cerr << "dlclose(" << DLOPEN_MODULE_PATH << ") failed: " << dlerror() << '\n';
    success = false;
  }

  captured.rdbuf()->pubsync();
  std::cout << "Captured output:\n" << captured.output_str() << std::endl;

  success = compare_dlopen_output(captured.input()) && success;
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
