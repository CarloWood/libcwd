// Exercise ordinary dc::notice logging for constructors and destructors in
// dependent and dlopen-loaded shared objects. This CTest fixture intentionally
// avoids ELF/DWARF diagnostics, symbol lookup, and file:line assertions; it only
// verifies that the expected DSOs load and expose their tiny probe functions.

#include "cwd_sys.h"
#include "location_loading_support.h"

#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <string>

extern "C" int location_libtest1_touch();

#ifndef LOCATION_PLUGIN2_PATH
#define LOCATION_PLUGIN2_PATH "location_plugin2.so"
#endif

namespace {

using plugin_touch_fn = int (*)();

// Compare the fixture's captured constructor/destructor output after the final
// shared-object destructor runs. The input stream is owned by
// location_loading_support.h; this function consumes it and exact-matches the
// deterministic lifecycle lines. A false return lets the support code abort so
// CTest sees failures that happen after main() has returned.
bool compare_location_loading_output(std::istream& captured)
{
  char const* expected[] = {
    "NOTICE  : location_libtest2.so: loaded",
    "NOTICE  : location_libtest1.so: loaded",
    "NOTICE  : location_plugin1.so: loaded",
    "NOTICE  : location_plugin2.so: loaded",
    "NOTICE  : location_plugin2.so: unloaded",
    "NOTICE  : location_libtest1.so: unloaded",
    "NOTICE  : location_libtest2.so: unloaded",
    "NOTICE  : location_plugin1.so: unloaded",
    nullptr
  };

  return libcwd_ctest::matches_expected_output(captured, expected);
}

// Resolve and call an integer probe symbol from an already-open object.  The
// function returns false on missing symbols or unexpected return values and
// writes a diagnostic to std::cerr; it has no libcwd side effects and does not
// close the handle passed by the caller.
bool check_probe(void* handle, char const* symbol_name, int expected)
{
  dlerror();
  void* symbol = dlsym(handle, symbol_name);
  char const* error = dlerror();
  if (error)
  {
    std::cerr << "dlsym(" << symbol_name << ") failed: " << error << '\n';
    return false;
  }

  plugin_touch_fn touch = reinterpret_cast<plugin_touch_fn>(symbol);
  int const value = touch();
  if (value != expected)
  {
    std::cerr << symbol_name << " returned " << value << ", expected " << expected << '\n';
    return false;
  }

  return true;
}

} // namespace

int main()
{
  Debug(main_reached());
  libcwd_ctest::location_loading::initialize_notice_logging();
  libcwd_ctest::location_loading::register_final_output_check(compare_location_loading_output);

  int const lib_value = location_libtest1_touch();
  if (lib_value != 12)
  {
    std::cerr << "location_libtest1_touch returned " << lib_value << ", expected 12\n";
    return EXIT_FAILURE;
  }

  if (!check_probe(RTLD_DEFAULT, "location_plugin1_touch", 101))
    return EXIT_FAILURE;

  void* plugin2 = dlopen(LOCATION_PLUGIN2_PATH, RTLD_NOW | RTLD_LOCAL);
  if (!plugin2)
  {
    std::cerr << "dlopen(location_plugin2.so) failed: " << dlerror() << '\n';
    return EXIT_FAILURE;
  }

  bool const plugin2_ok = check_probe(plugin2, "location_plugin2_touch", 202);
  if (dlclose(plugin2) != 0)
  {
    std::cerr << "dlclose(location_plugin2.so) failed: " << dlerror() << '\n';
    return EXIT_FAILURE;
  }

  return plugin2_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
