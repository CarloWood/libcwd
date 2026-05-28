// default-argument call sites used by location_inlined_calls.cc.
//
// g has default arguments that call y_default and arg2_i, arg2_i is nested inside
// another inline helper, and f calls g with explicit arguments arg_g1 and arg_g2.
// Each helper receives the logical line that a location lookup should report for
// the helper's "return address plus libcwd::builtin_return_address_offset",
// where libcwd::builtin_return_address_offset is typically -1, that is assumed to
// be a byte that is part of the call location.

#include "cwd_sys.h"
#include "test_support.h"
#include <cstddef>

extern int a;
extern int arg_g1(int expected_line), arg_g2(int expected_line), y_default(int expected_line), arg2_i(int expected_line);

int expected_line;      // Expected call line of default argument initialization functions.

inline int
i(int x, int y)
{
  a += x + y;
  return a;
}

inline bool
g(int x,
  int y = y_default(expected_line),
  int z = i(12345, arg2_i(expected_line)))
{
  libcwd::location_ct g_loc((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
  Dout(dc::notice, "g(" << x << ", " << y << ", " << z << ") was called from " << g_loc);

  int q[4] = { 2, 3, 5, 7 };
  a += x + y + z + reinterpret_cast<ptrdiff_t>(&q);
  return x == 1 && y == 1 && z == 1;
}

bool f()
{
  libcwd::location_ct const g_loc(&&g_call_location);
  expected_line = __LINE__ + 3; // Same as line below g_call_location.
  Dout(dc::notice, "calling g(arg_g1(), arg_g2()) from f: " << g_loc << " (expected: " << expected_line << ")");
g_call_location:
  auto result_g = g(            // Line 45 : this is the location from which the default argument initializing functions are called from.
        arg_g1(__LINE__),       // Line 46 : this is where arg_g1 is called from.
        arg_g2(__LINE__)        // Line 47 : this is where arg_g2 is called from.
    );
  return g_loc.line() == expected_line && result_g;
}
