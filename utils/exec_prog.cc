// $Header$
//
// Copyright (C) 2000, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include <libcw/sys.h>
#include <sys/poll.h>		// Needed for poll(2)
#include <unistd.h>		// Needed for pipe(2), dup2(2), fork(2), read(2) and close(2)
#include <errno.h>		// Needed for errno
#include <signal.h>		// Needed for kill(2) and SIGKILL
#include <sys/wait.h>		// Needed for waitpid(2)
#include <libcw/exec_prog.h>
#include <libcw/debug.h>
#ifdef CWDEBUG
#include <libcw/buf2str.h>
#include <libcw/cwprint.h>
#include <fstream>
#ifdef __GLIBCPP__
#include <streambuf>
#endif
#endif

RCSTAG_CC("$Id$")

namespace libcw {
  namespace debug {

#ifdef CWDEBUG
    namespace {
      void print_poll_array_on(std::ostream& os, struct pollfd const ptr[2], size_t size);
    }
#endif

    // 
    // Execute a program (blocking).
    //
    // Return value is the return value of the program, or 256 if the
    // program was terminated because `decode_stdout' returned -1.
    //
    // This function executes program `prog_name' with arguments `argv' and environment `envp'
    // and reads stdout of the child process.  The function blocks until the child process exits
    // (or until it closed both stdout and stderr) or until `decode_stdout' returns -1.  Each
    // line (ending on a new-line) received via stdout from the child process is passed to the
    // call-back function `decode_stdout'.
    //

    int exec_prog(char const* prog_name, char const* const argv[], char const* const envp[], int (*decode_stdout)(char const*, size_t))
    {
      Debug(
	if (!dc::debug.is_on())
	  libcw_do.off()
      );
      Dout(dc::debug|continued_cf, "exec_prog(\"" << prog_name << "\", " <<
          cwprint(argv_ct(argv)) << ", " << cwprint(environment_ct(envp)) << ", decode) = ");

      int stdout_filedes[2];
      int stderr_filedes[2];
#ifdef CWDEBUG
      int debug_filedes[2];
#endif

      int ret;
      
      if ((ret = pipe(stdout_filedes)) == -1)
	DoutFatal(dc::fatal|error_cf, "pipe");
      Dout(dc::system, "pipe([" << stdout_filedes[0] << ", " << stdout_filedes[1] << "]) = " << ret);
      if ((ret = pipe(stderr_filedes)) == -1)
	DoutFatal(dc::fatal|error_cf, "pipe");
      Dout(dc::system, "pipe([" << stderr_filedes[0] << ", " << stderr_filedes[1] << "]) = " << ret);
#ifdef CWDEBUG
      if ((ret = pipe(debug_filedes)) == -1)
	DoutFatal(dc::fatal|error_cf, "pipe");
#ifdef DEBUGDEBUG
      Dout(dc::system, "pipe([" << debug_filedes[0] << ", " << debug_filedes[1] << "]) = " << ret);
#endif
#endif

      Debug( libcw_do.off() );
      ret = fork();

      switch(ret)
      {
	case 0:
	{
#ifdef CWDEBUG
#ifdef __GLIBCPP__
	  // Quick and dirty unbuffered file descriptor streambuf.
	  class fdbuf : public std::basic_streambuf<char, std::char_traits<char> > {
	  public:
	    typedef std::char_traits<char> traits_type;
	    typedef traits_type::int_type int_type;
	  private:
	    int M_fd;
	  public:
	    fdbuf(int fd) : M_fd(fd) { }
          protected:
	    virtual int_type overflow(int_type c = traits_type::eof())
	    {
	      if (!traits_type::eq_int_type(c, traits_type::eof()))
	      {
		char cp[1];
		*cp = c;
		if (write(M_fd, cp, 1) != 1)
		  return traits_type::eof();
	      }
	      return 0;
	    }
	    // This would be needed if it was buffered.
	    // virtual std::streamsize xsputn(char const* s, std::streamsize num) { return write(M_fd, s, num); }
	  };

	  // Unbuffered file descriptor stream.
	  class ofdstream : public std::ostream {
	  private:
	    mutable fdbuf M_fdbuf;
	  public:
	    explicit
	    ofdstream(int fd) : std::ostream(&M_fdbuf), M_fdbuf(fd) { }
	    fdbuf* rdbuf(void) const { return &M_fdbuf; }
	  };
	  
	  // Write debug output to pipe.
	  ofdstream debug_stream(debug_filedes[1]);
#else
	  std::ofstream debug_stream(debug_filedes[1]);
#endif
	  Debug( libcw_do.set_margin(std::string(prog_name) + ": ") );
	  Debug( libcw_do.set_ostream(&debug_stream) );
	  Debug( libcw_do.on() );
#endif
	  // Child process
	  Dout(dc::system, "fork() = 0 [child process]");
	  ret = close(stdout_filedes[0]);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << stdout_filedes[0] << ") = " << ret);
	  ret = close(stderr_filedes[0]);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << stderr_filedes[0] << ") = " << ret);
#ifdef CWDEBUG
	  ret = close(debug_filedes[0]);
#ifdef DEBUGDEBUG
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << debug_filedes[0] << ") = " << ret);
#endif
#endif
	  ret = close(1);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(1) = " << ret);
	  ret = close(2);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(2) = " << ret);
	  ret = dup2(stdout_filedes[1], 1);
	  Dout(dc::system|cond_error_cf(ret == -1), "dup2(" << stdout_filedes[1] << ", 1) = " << ret);
	  ret = dup2(stderr_filedes[1], 2);
	  Dout(dc::system|cond_error_cf(ret == -1), "dup2(" << stderr_filedes[1] << ", 2) = " << ret);
	  ret = close(stdout_filedes[1]);
	  Dout(dc::system|flush_cf|cond_error_cf(ret == -1), "close(" << stdout_filedes[1] << ") = " << ret);
	  ret = close(stderr_filedes[1]);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << stderr_filedes[1] << ") = " << ret);

	  execve(prog_name, const_cast<char* const*>(argv), const_cast<char* const*>(envp));
	}
	case -1:
	  Debug( libcw_do.on() );
	  Dout(dc::system|error_cf, "fork() = -1");
	  DoutFatal(dc::fatal, "Try undefining DEBUGUSEBFD");
	default:
	{
	  pid_t pid = ret;
	  Debug( libcw_do.on() );
	  // Parent process
	  Dout(dc::system, "fork() = " << ret << " [parent process]");
	  ret = close(stdout_filedes[1]);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << stdout_filedes[1] << ") = " << ret);
	  ret = close(stderr_filedes[1]);
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << stderr_filedes[1] << ") = " << ret);
#ifdef CWDEBUG
	  ret = close(debug_filedes[1]);
#ifdef DEBUGDEBUG
	  Dout(dc::system|cond_error_cf(ret == -1), "close(" << debug_filedes[1] << ") = " << ret);
#endif
#endif

#ifdef CWDEBUG
	  int const max_number_of_fds = 3;
#else
	  int const max_number_of_fds = 2;
#endif
	  int number_of_fds = max_number_of_fds;
	  struct pollfd ufds[max_number_of_fds];
	  std::string decodebuf[max_number_of_fds];
	  ufds[0].fd = stdout_filedes[0];
	  ufds[0].events = POLLIN;
	  ufds[1].fd = stderr_filedes[0];
	  ufds[1].events = POLLIN;
#ifdef CWDEBUG
	  ufds[2].fd = debug_filedes[0];
	  ufds[2].events = POLLIN;
#endif
	  do
	  {
	    Dout(dc::system|continued_cf, "poll(");
	    ret = poll(ufds, number_of_fds, -1);
	    Debug(
	      if (libcw_do._off < 0)
		if ((libcw_do|dc::continued).on)
		  print_poll_array_on(*libcw_do.current_oss, ufds, number_of_fds);
	    );
	    Dout(dc::finish|cond_error_cf(ret == -1), ", " << number_of_fds << ", -1) = " << ret);
	    if (ret == -1)
	    {
	      if (errno == EINTR)
		continue;
	      break;
	    }
	    for (int fd = 0; fd < number_of_fds; ++fd)
	    {
	      if ((ufds[fd].revents & POLLIN) == POLLIN)
	      {
		char readbuf[128];
		int len;
		do
		{
#ifdef CWDEBUG
#ifndef DEBUDEBUG
		  if (ufds[fd].fd != debug_filedes[0])
#endif
		    Dout(dc::system|continued_cf, "read(" << ufds[fd].fd << ", ");
#endif
		  len = read(ufds[fd].fd, readbuf, sizeof(readbuf));
#ifdef CWDEBUG
#ifndef DEBUDEBUG
		  if (ufds[fd].fd != debug_filedes[0])
#endif
		    Dout(dc::finish|cond_error_cf(len == -1), '"' << buf2str(readbuf, len > 0 ? len : 0) << "\", " << sizeof(readbuf) << ") = " << len);
#endif
		  if (len <= 0)
		  {
		    ufds[fd].revents = POLLHUP;	// Needed on FreeBSD which doesn't detect this via poll()
		    break;
		  }
		  for (char const* p = readbuf; p < &readbuf[len]; ++p)
		  {
		    decodebuf[fd] += *p;
		    if (*p == '\n')
		    {
		      // Process decode buf
		      if (ufds[fd].fd == stdout_filedes[0])
		      {
			if (decode_stdout(decodebuf[fd].data(), decodebuf[fd].size()) == -1)
			{
			  Dout(dc::notice, "decode_stdout() returned -1, terminating child process.");
			  Dout(dc::system|continued_cf, "kill(" << pid << ", SIGKILL) = ");
			  ret = kill(pid, SIGKILL);
			  Dout(dc::finish|cond_error_cf(ret == -1), ret);
			  for (int fd = 0; fd < number_of_fds; ++fd)
			  {
			    ret = close(ufds[fd].fd);
			    Dout(dc::system|cond_error_cf(ret == -1), "close(" << ufds[fd].fd << ") = " << ret);
			  }
			  number_of_fds = 0;
			  ret = 256;
			  len = 1;
			  readbuf[len - 1] = '\n';
			  break;
			}
		      }
		      else if (ufds[fd].fd == stderr_filedes[0])
			Dout(dc::warning, "child process returns on stderr: \"" << buf2str(decodebuf[fd].data(), decodebuf[fd].size()) << '"');
#ifdef CWDEBUG
		      else if (ufds[fd].fd == debug_filedes[0])
		      {
			Debug( libcw_do.get_ostream()->flush() );
			Debug( libcw_do.get_ostream()->write(decodebuf[fd].data(), decodebuf[fd].size()) );
			Debug( libcw_do.get_ostream()->flush() );
		      }
#endif
		      decodebuf[fd].erase();
		    }
		  }
		}
		while(readbuf[len - 1] != '\n');
		if (len > 0)
		  continue;	// Ignore possible errors when data was read.
	      }
	      if ((ufds[fd].revents & (POLLERR|POLLHUP|POLLNVAL)))
	      {
		--number_of_fds;
		if (fd < number_of_fds)
		{
		  ufds[fd] = ufds[number_of_fds];
		  decodebuf[fd] = decodebuf[number_of_fds];
		}
		if (number_of_fds == 0)
		{
		  int status;
		  Dout(dc::system|continued_cf, "waitpid(" << pid << ", { ");
		  ret = waitpid(pid, &status, 0);
		  Dout(dc::finish|cond_error_cf(ret == -1), std::hex << status << " }, 0) = " << std::dec << ret);
		  if (WIFEXITED(status))
		    ret = WEXITSTATUS(status);
		  else
		  {
		    Dout(dc::warning, "exec_prog: Child process \"" << prog_name << "\" terminated abnormally.");
		    ret = -1;
		  }
		}
		break;
	      }
	    }
	  }
	  while(number_of_fds > 0);
	  break;
	}
      }

      Dout(dc::finish, ret);
      Debug(
	if (!dc::debug.is_on())
	  libcw_do.on()
      );
      return ret;
    }

#ifdef CWDEBUG
    namespace {		// Implementation of local functions

      // {anonymous}::
      static void print_poll_struct_on(std::ostream& os, struct pollfd const& pfd)
      {
	os << "{ " << pfd.fd << ", ";
	short const* eventp = &pfd.events;
	while(true)
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
      void print_poll_array_on(std::ostream& os, struct pollfd const ptr[2], size_t size)
      {
	os << " [ ";
	if (size > 0)
	  print_poll_struct_on(os, ptr[0]);
	for (size_t i = 1; i < size; ++i)
	{
	  os << ", ";
	  print_poll_struct_on(os, ptr[i]);
	}
	os << " ]";
      }

    } // namespace {anonymous}

  } // namespace debug
} // namespace libcw

#endif // CWDEBUG
