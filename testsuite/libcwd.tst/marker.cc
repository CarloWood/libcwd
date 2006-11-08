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
#include "alloctag_debug.h"
#include <iostream>

// A dummy class
class Am {
  int i;
  int j;
  char k;
};

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_ALLOC || !CWDEBUG_LOCATION || !CWDEBUG_MARKER
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );
#if CWDEBUG_ALLOC && !defined(THREADTEST)
  int* dummy = new int;					// Make sure initialization of libcwd is done.
  libcwd::make_all_allocations_invisible_except(NULL);	// Don't show allocations that are done as part of initialization.
#endif
#if CWDEBUG_LOCATION
  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)exit) );
#endif

  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
  Debug( dc::notice.on() );
  Debug( dc::malloc.on() );
  Debug( dc::warning.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() ); 

  // Allocate new object
  Am* a1 = new Am;
  AllocTag(a1, "First created");

#if CWDEBUG_MARKER
  // Create marker
  libcwd::marker_ct* marker = new libcwd::marker_ct("A test marker");
#endif

  // Allocate more objects
  Am* a2 = new Am[10];
  AllocTag(a2, "Created after the marker");
  int* p = new int[30];
  AllocTag(p, "Created after the marker");

  // Show Memory Allocation Overview
  Debug( list_allocations_on(libcw_do) );

#if CWDEBUG_MARKER
  Dout(dc::notice, "Moving the int array outside of the marker...");
  Debug( move_outside(marker, p) );
#endif

  // Show Memory Allocation Overview
  Debug( list_allocations_on(libcw_do) );

#if CWDEBUG_MARKER
  // Delete the marker
  delete marker;
#endif

  delete [] p;
  delete [] a2;
  delete a1;

  Dout(dc::notice, "Finished successfully.");

  Debug( libcw_do.off() );

#if CWDEBUG_ALLOC && !defined(THREADTEST)
  delete dummy;
#endif

  EXIT(0);
}
