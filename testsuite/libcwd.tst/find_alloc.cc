// $Header$
//
// Copyright (C) 2002 - 2003, by
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
#include <libcwd/debug.h>
#include <iostream>

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_ALLOC
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );
#if CWDEBUG_ALLOC && !defined(THREADTEST)
  int* dummy = new int;					// Make sure initialization of libcwd is done.
  libcwd::make_all_allocations_invisible_except(NULL);	// Don't show allocations that are done as part of initialization.
#endif
  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
  Debug( dc::notice.on() );
  Debug( dc::malloc.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() ); 

#if CWDEBUG_ALLOC
  char* p1 = new char[4];
  char* p2 = new char[5];

  if (libcwd::find_alloc(p1) != libcwd::find_alloc(p1 + 3))
    DoutFatal( dc::core, "find_alloc failed." );
  if (libcwd::find_alloc(p1 + 4))
    DoutFatal( dc::core, "find_alloc should have returned NULL" );
  if (!libcwd::find_alloc(p2 + 4))
    DoutFatal( dc::core, "find_alloc should have returned non-NULL" );
#endif

  Dout( dc::notice, "Finished successfully." );
  Debug( libcw_do.off() );

  delete [] p2;
  delete [] p1;
  delete dummy;

  EXIT(0);
}
