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
  struct stat buf;

  // Warning: this is NOT the correct way to do this (see below)
  Dout(dc::notice|nonewline_cf,
       "stat(\"" << file_name << "\", ");

  int ret = stat(file_name, &buf);

  Dout(dc::notice|noprefix_cf|cond_error_cf(ret != 0),
       &buf << ") = " << ret);
}
