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
#include <libcw/h.h>
#include <libcw/debug.h>
#include <libcw/debugdebugcheckpoint.h>

#ifdef DEBUGDEBUG
RCSTAG_CC("$Id$")

void debugdebugcheckpoint(void)
{
  static int counter = 0;
#if 0
  cerr << endl << "Entering debugdebugcheckpoint(void)" << endl;

  static bool init = false;
  void const* address = 0x8010a20;	 	// The memory address we watch
  void const* currupted_value = 0xffff0000;	// The value that doesn't smell well
  int const cv = 0;				// Do not access memory address until counter reached this value

  if (counter < cv)
    cout << "Counting (" << counter << ")" << endl;
  else
  {
    if (init)
      cout << "checking! ";
    else
      cout << "waiting ";
    cout << "(" << counter << ") " << "current value: " << *((void**)address) << endl;
    if (init && *((void**)address) == (void*)currupted_value)
    {
      cout << "Coredumping!" << endl;
      MyCoreDump
    }
    else if (!init && *((void**)address) != (void*)currupted_value)
    {
      init = true;
      cout << "init set to true!" << endl;
    }
  }
#endif
  counter++;
}
#else

#if 0	// Not needed anymore for gcc-2.95.1
// A bug in egcs-1.1.1 causes the linker error:
// ./debugging/debugdebugcheckpoint.o: In function `global constructors keyed to int lexicographical_compare_3way<signed char const*, signed char const*>(signed char const*, signed char const*, signed char const*, signed char const*)':
// /usr/local/egcs/include/g++/stl_algobase.h:427: multiple definition of `global constructors keyed to int lexicographical_compare_3way<signed char const*, signed char const*>(signed char const*, signed char const*, signed char const*, signed char const*)'
// ./llists/cbll.o:/usr/local/egcs/include/g++/stl_algobase.h:427: first defined here
//
// According to Martin v. Loewis (egcs developer):
// Thanks. This is a known bug. The work-around is to put some global
// symbol into the source file. If this still fails, the global symbol
// should appear before the include. Eg.
//
// int this_is_debugdebugcheckpoint_cc = 0;

int bug_workaround_for_empty_debugdebugcheckpoint_cc = 0;
#endif

#endif // DEBUGDEBUG
