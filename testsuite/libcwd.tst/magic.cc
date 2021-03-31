// $Header$
//
// Copyright (C) 2000 - 2003, by
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
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include "alloctag_debug.h"

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_ALLOC || !CWDEBUG_MAGIC
  DoutFatal(dc::fatal, "Failure Expected.");
#endif

  Debug( check_configuration() );

  // Don't show allocations that are allocated before main()
  libcwd::make_all_allocations_invisible_except(NULL);

  // Select channels
  Debug( dc::malloc.off() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() );

  int* p = new int[4];
  AllocTag(p, "Test array");
  Debug( list_allocations_on(libcw_do) );
  p[4] = 5;     // Deliberate buffer overflow.

  pid_t pid = fork();
  if (pid == -1)
    DoutFatal(dc::fatal|error_cf, "fork");
  if (pid == 0)
  {
    // This must core dump:
    delete[] p;
    DoutFatal(dc::fatal, "This should not be reached!");
  }
  // Parent process
  int status;
  EXIT((pid == wait(&status) && WIFSIGNALED(status)) ? 0 : 1);
}
