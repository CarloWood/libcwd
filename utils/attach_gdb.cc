// $Header$
//
// Copyright (C) 2003 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include "cwd_debug.h"
#include "libcwd/debug.h"

#ifdef _WIN32

namespace libcwd {

void attach_gdb()
{
  DoutFatal(dc::core, "attach_gdb() is not supported on windows");
}

}

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cerrno>
#include <time.h>
#include <fstream>
#include <cstdlib>

int libcwd_attach_gdb_hook = 0;

namespace libcwd {

void attach_gdb()
{
  pid_t pid1 = getpid();
  std::ofstream f;
  f.open("gdb.cmds");
//  f << "set scheduler-locking step\nhandle SIG34 nostop\nset substitute-path /build/gcc-8-sYUbZB/gcc-8-8.2.0/build/x86_64-linux-gnu/libstdc++-v3/include /usr/include/c++/8\n";
  f << "b *" << __builtin_return_address(0) << "\nset (int)libcwd_attach_gdb_hook=0\nc\n";
  f.close();
  char command[256];
  Dout(dc::always, "gdb = \"" << rcfile.gdb_bin() << "\".");
  size_t len = snprintf(command, sizeof(command), "%s -n -x gdb.cmds /proc/%u/exe %u", rcfile.gdb_bin().c_str(), pid1, pid1);
  if (len >= sizeof(command))
    DoutFatal(dc::fatal, "rcfile: value of keyword 'gdb' too long (" << rcfile.gdb_bin() << ')');
  if (rcfile.gdb_bin().size() == 0)
    DoutFatal(dc::fatal, "rcfile: value of keyword 'gdb' is empty. Did you call Debug(read_rcfile()) at all?");
  char command2[512];
  Dout(dc::always, "xterm = \"" << rcfile.konsole_command() << "\".");
  len = snprintf(command2, sizeof(command2), rcfile.konsole_command().c_str(), command);
  Dout(dc::always, "Executing \"" << command2 << "\".");
  if (len >= sizeof(command2))
    DoutFatal(dc::fatal, "rcfile: value of keyword 'xterm' too long (" << rcfile.konsole_command());
  libcwd_attach_gdb_hook = 1;
  pid_t pid2 = fork();
  Debug(libcw_do.off());
  switch(pid2)
  {
    case -1:
      Debug(libcw_do.on());
      DoutFatal(dc::fatal|error_cf, "fork()");
    case 0:
    {
      int ret = system(command2);
      _exit(ret == -1 ? 255	// system failed (ie, fork failed).
	             : WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT) ? 126	// Terminated by signal (127 means that /bin/sh failed).
		     : WEXITSTATUS(ret));	// Return value of command2.
    }
    default:
    {
      Debug(libcw_do.on());
      struct timespec t = { 0, 100000000 };
      int loop = 0;
      while (libcwd_attach_gdb_hook)
      {
        if (++loop > 50)	// Calling waitpid turns out to stop gdb from being able to attach.
				// If after 5 seconds gdb still didn't do it's thing, check if it's
				// still running - if not, print an error.
	{
	  //Dout(dc::notice, "Entering waitpid");
	  int status;
	  pid_t pid3 = waitpid(pid2, &status, WNOHANG);
	  if (pid3 == pid2 || ((int)pid3 == -1 && errno == ECHILD))
	  {
	    libcwd_attach_gdb_hook = 0;
	    if (WIFEXITED(status))
	      DoutFatal(dc::core, "Failed to start gdb: 'xterm' terminated with exit code " << WEXITSTATUS(status) <<
		  " before attaching to the process. This can happen when you call attach_gdb from "
		  "the destructor of a global object. It also happens when gdb fails to attach, "
		  "for example because you already run the application inside gdb.");
	    else if (WIFSIGNALED(status))
	      DoutFatal(dc::core, "Failed to start gdb: 'xterm' terminated because of (uncaught) signal " << WTERMSIG(status) <<
		  " before attaching to the process.");
#ifdef WCOREDUMP
	    else if (WCOREDUMP(status))
	      DoutFatal(dc::core, "Failed to start gdb: 'xterm' dumped core before attaching to the process.");
#endif
	    DoutFatal(dc::core, "Failed to start gdb: 'xterm' terminated before attaching to the process.");
	  }
	}
	nanosleep(&t, NULL);
      }
      Dout(dc::always, "ATTACHED!");
    }
  }
}

} // namespace libcwd

#endif // !_WIN32
