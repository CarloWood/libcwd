#include "cwd_sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "environment.h"
#include <libcwd/buf2str.h>
#include <libcwd/cwprint.h>
#include <libcwd/debug.h>
#include <libcwd/private_assert.h>
#include <libcwd/private_string.h>

#include <cerrno> // Needed for errno
#include <fstream>
#include <streambuf>
#include <signal.h> // Needed for kill(2) and SIGKILL
#include <sys/poll.h> // Needed for poll(2)
#include <sys/types.h> // Needed for waitpid(2)
#include <sys/wait.h> //
#include <unistd.h> // Needed for pipe(2), dup2(2), fork(2), read(2) and close(2)
#include "cwd_debug.h"

namespace libcwd {

namespace {
class Argv
{
 private:
  char const* const* __argv;

 public:
  Argv(char const* const argv[]) : __argv(argv) { }
  void print_on(std::ostream& os) const;
};

class PrintablePollDummy
{
 private:
  struct pollfd const* M_pollfds;
  size_t M_number_of_fds;

 public:
  PrintablePollDummy(struct pollfd const* pollfds, size_t number_of_fds)
      : M_pollfds(pollfds), M_number_of_fds(number_of_fds)
  {
  }
  void print_on(std::ostream& os) const;
};

} // namespace

namespace { // Implementation of local functions

// {anonymous}::
static void print_poll_struct_on(std::ostream& os, struct pollfd const& pfd)
{
  os << "{ " << pfd.fd << ", ";
  short const* eventp = &pfd.events;
  while (true)
  {
    short event = *eventp;
    if (!event)
      os << "0";
    if ((event & POLLIN))
    {
      os << "POLLIN";
      event &= ~POLLIN;
      if (event)
        os << '|';
    }
    if ((event & POLLPRI))
    {
      os << "POLLPRI";
      event &= ~POLLPRI;
      if (event)
        os << '|';
    }
    if ((event & POLLOUT))
    {
      os << "POLLOUT";
      event &= ~POLLOUT;
      if (event)
        os << '|';
    }
    if ((event & POLLERR))
    {
      os << "POLLERR";
      event &= ~POLLERR;
      if (event)
        os << '|';
    }
    if ((event & POLLHUP))
    {
      os << "POLLHUP";
      event &= ~POLLHUP;
      if (event)
        os << '|';
    }
    if ((event & POLLNVAL))
    {
      os << "POLLNVAL";
      event &= ~POLLNVAL;
      if (event)
        os << '|';
    }
    if (event)
      os << std::hex << event;
    if (eventp == &pfd.revents)
    {
      os << " }";
      break;
    }
    os << ", ";
    eventp = &pfd.revents;
  }
}

// {anonymous}::
void PrintablePollDummy::print_on(std::ostream& os) const
{
  os << " [ ";
  if (M_number_of_fds > 0)
    print_poll_struct_on(os, M_pollfds[0]);
  for (size_t i = 1; i < M_number_of_fds; ++i)
  {
    os << ", ";
    print_poll_struct_on(os, M_pollfds[i]);
  }
  os << " ]";
}

// {anonymous}::
void Argv::print_on(std::ostream& os) const
{
  os << "[ ";
  char const* const* p = __argv;
  while (*p) os << *p++ << ", ";
  os << "NULL ]";
}

} // namespace

} // namespace libcwd

#endif // CWDEBUG_LOCATION
