#include "cwd_sys.h"

#include <match.h>

namespace libcwd::_private_ {

bool match(char const* mask, size_t masklen, char const* name)
{
  char const* m = mask;
  char const* n = name;

  for (;;)
  {
    if (*n == 0)
    {
      while (masklen-- > 0)
        if (*m++ != '*')
          return false;
      return true;
    }
    if (*m == '*')
      break;
    if (*n++ != *m++)
      return false;
    --masklen;
  }
  while (--masklen > 0 && *++m == '*');
  if (masklen == 0)
    return true;
  while (*n != *m || !match(m, masklen, n))
    if (*n++ == 0)
      return false;
  return true;
}

} // namespace libcwd::_private_
