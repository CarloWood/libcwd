// Inline/default-argument call sites used by location_inlined_calls.cc.
//
// The inline functions below intentionally keep the historical tests/inlined.cc
// shape: g has default arguments that call y_default and arg2_i, arg2_i is nested inside
// another inline helper, and f calls g with an explicit arg_g argument.  Each helper
// receives the logical line that a fully inline-aware location lookup should
// report for the helper's return address.

#include "cwd_sys.h"
#include "test_support.h"
#include <cstddef>

extern int a;
extern int y_default(int expected_line), arg2_i(int expected_line), arg_g(int expected_line);

inline int
i(int x, int y)
{
  a += x + y;
  return a;
}

inline bool
g(int x,
  int y = y_default(__LINE__),
  int z = i(12345, arg2_i(__LINE__)))
{
  int q[4] = { 2, 3, 5, 7 };
  a += x + y + z + reinterpret_cast<ptrdiff_t>(&q);
  return x == 1 && y == 1 && z == 1;
}

bool f()
{
  libcwd::location_ct const g_loc(&&current_location);
  int const expected_line = __LINE__ + 3;
  Dout(dc::notice, "calling g(arg_g()) from f: " << g_loc << " (expected: " << expected_line << ")");
current_location:
  return g_loc.line() == expected_line && g(arg_g(__LINE__));
}
