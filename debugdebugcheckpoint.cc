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
      raise(3);
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

#endif // DEBUGDEBUG
