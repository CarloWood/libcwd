// $Header$
//
// Copyright (C) 2002, by
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
#include <libcw/debug.h>
#include <iostream>

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_ALLOC
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );
#if CWDEBUG_ALLOC && !defined(THREADTEST)
  new int;							// Make sure initialization of libcwd is done.
  libcw::debug::make_all_allocations_invisible_except(NULL);	// Don't show allocations that are done as part of initialization.
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

  if (libcw::debug::find_alloc(p1) != libcw::debug::find_alloc(p1 + 3))
    DoutFatal( dc::core, "find_alloc failed." );
  if (libcw::debug::find_alloc(p1 + 4))
    DoutFatal( dc::core, "find_alloc should have returned NULL" );
  if (!libcw::debug::find_alloc(p2 + 4))
    DoutFatal( dc::core, "find_alloc should have returned non-NULL" );
#endif

  Dout( dc::notice, "Finished successfully." );
  Debug( libcw_do.off() );

  EXIT(0);
}
