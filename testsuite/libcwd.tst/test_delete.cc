// $Header$
//
// Copyright (C) 2000 - 2002, by
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

class At {};

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_ALLOC || (!CWDEBUG_LOCATION && defined(CWDEBUG))
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

  // Allocate new object
  At* a = new At;
  AllocTag(a, "Test object that we will make invisible");

#if CWDEBUG_ALLOC
  // Check test_delete
  if (libcw::debug::test_delete(a))	// Should return false
    DoutFatal( dc::core, "CANNOT find that pointer?!" );
  if (!libcw::debug::find_alloc(a))
    DoutFatal( dc::core, "CANNOT find that pointer?!" );
#endif

  // Show Memory Allocation Overview
  Dout( dc::notice, "Before making allocation invisible:" );
  Debug( list_allocations_on(libcw_do) );

  // Make allocation invisible
  libcw::debug::make_invisible(a);

  // Show Memory Allocation Overview
  Dout( dc::notice, "After making allocation invisible:" );
  Debug( list_allocations_on(libcw_do) );

#if CWDEBUG_ALLOC
  // Check test_delete
  if (libcw::debug::test_delete(a))	// Should still return false
    DoutFatal( dc::core, "CANNOT find that pointer?!" );
  if (libcw::debug::find_alloc(a))
    DoutFatal( dc::core, "Can STILL find that pointer?!" );
  // Test set_alloc_checking_off/set_alloc_checking_on, using test_delete
  LIBCWD_TSD_DECLARATION;
  libcw::debug::_private_::set_alloc_checking_off(LIBCWD_TSD);
  char* ptr = (char*)malloc(100);
  libcw::debug::_private_::set_alloc_checking_on(LIBCWD_TSD);
  if (!libcw::debug::test_delete(ptr))
    DoutFatal( dc::core, "Can find an INTERNAL pointer?!" );
  libcw::debug::_private_::set_alloc_checking_off(LIBCWD_TSD);
  free(ptr);
  libcw::debug::_private_::set_alloc_checking_on(LIBCWD_TSD);
#endif

  Dout( dc::notice, "Finished successfully." );
#ifndef CWDEBUG
  std::cout << "Finished successfully.\n";
#endif

  Debug( libcw_do.off() );

  EXIT(0);
}
