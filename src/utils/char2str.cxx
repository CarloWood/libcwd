#include "cwd_sys.h"
#include <libcwd/char2str.h>

#include <iostream>

namespace libcwd {

namespace {
char const c2s_tab[7] = {'a', 'b', 't', 'n', 'v', 'f', 'r'};
}

void char2str::print_char_to(std::ostream& os) const
{
  os.put(c);
}

void char2str::print_escaped_char_to(std::ostream& os) const
{
  os.put('\\');
  if (c > 6 && c < 14)
  {
    os.put(c2s_tab[c - 7]);
    return;
  }
  else if (c == 27)
  {
    os.put('e');
    return;
  }
  else if (c == '\\')
  {
    os.put('\\');
    return;
  }
  os.put('x');
  int xval = (unsigned char)c;
  int hd = xval / 16;
  for (int i = 0; i < 2; ++i)
  {
    os.put((char)((hd < 10) ? '0' + hd : 'A' + hd - 10));
    hd = xval % 16;
  }
}

} // namespace libcwd
