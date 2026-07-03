#include "debug.h"
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>

std::ostream& operator<<(std::ostream& os, struct stat const buf)
{
  os << "inode:" << buf.st_ino << "; " << "size:" << buf.st_size;
  return os;
}

std::ostream& operator<<(std::ostream& os, struct stat const* bufp)
{
  os << "{ " << *bufp << " }";
  return os;
}

// We only use this function to show what happens with the debug output,
// you shouldn't do anything like this in a real program.
int stat_with_buf_alloc(char const* file_name, struct stat*& bufp)
{
  bufp = new struct stat;
  return stat(file_name, bufp);
}

int main(int argc, char* argv[])
{
  Debug(main_reached());
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <file_name>\n";
    exit(-1);
  }

  char const* file_name = argv[1];
  struct stat* bufp;

  // This is NOT the correct way to do this.
  Dout(dc::notice|nonewline_cf,
       "stat_with_buf_alloc(\"" << file_name << "\", ");

  int ret = stat_with_buf_alloc(file_name, bufp);

  Dout(dc::notice|noprefix_cf|cond_error_cf(ret != 0),
       bufp << ") = " << ret);

  delete bufp;
}
