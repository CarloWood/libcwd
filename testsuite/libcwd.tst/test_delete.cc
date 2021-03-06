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

class At {};

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_ALLOC || (!CWDEBUG_LOCATION && defined(CWDEBUG))
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );
  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
  Debug( dc::notice.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() ); 
  Dout(dc::notice, "This write causes memory to be allocated.");
#if CWDEBUG_ALLOC && !defined(THREADTEST)
  int* dummy = new int;					// Make sure initialization of libcwd is done.
  libcwd::make_all_allocations_invisible_except(NULL);	// Don't show allocations that are done as part of initialization.
#endif
  Debug( dc::malloc.on() );

  // Allocate new object
  At* a = new At;
  AllocTag(a, "Test object that we will make invisible");

#if CWDEBUG_ALLOC
  // Check test_delete
  if (libcwd::test_delete(a))	// Should return false
    DoutFatal( dc::core, "CANNOT find that pointer?!" );
  if (!libcwd::find_alloc(a))
    DoutFatal( dc::core, "CANNOT find that pointer?!" );
#endif

  // Show Memory Allocation Overview
  Dout( dc::notice, "Before making allocation invisible:" );
  Debug( list_allocations_on(libcw_do) );

  // Make allocation invisible
  libcwd::make_invisible(a);

  // Show Memory Allocation Overview
  Dout( dc::notice, "After making allocation invisible:" );
  Debug( list_allocations_on(libcw_do) );

#if CWDEBUG_ALLOC
  // Check test_delete
  if (libcwd::test_delete(a))	// Should still return false
    DoutFatal( dc::core, "CANNOT find that pointer?!" );
  if (libcwd::find_alloc(a))
    DoutFatal( dc::core, "Can STILL find that pointer?!" );
  // Test set_alloc_checking_off/set_alloc_checking_on, using test_delete
  LIBCWD_TSD_DECLARATION;
  libcwd::_private_::set_alloc_checking_off(LIBCWD_TSD);
  char* ptr = (char*)malloc(100);
  libcwd::_private_::set_alloc_checking_on(LIBCWD_TSD);
  if (!libcwd::test_delete(ptr))
    DoutFatal( dc::core, "Can find an INTERNAL pointer?!" );
  libcwd::_private_::set_alloc_checking_off(LIBCWD_TSD);
  free(ptr);
  libcwd::_private_::set_alloc_checking_on(LIBCWD_TSD);
#endif

  Dout( dc::notice, "Finished successfully." );
#ifndef CWDEBUG
  std::cout << "Finished successfully.\n";
#endif

  delete a;
  Debug( libcw_do.off() );

#if CWDEBUG_ALLOC && !defined(THREADTEST)
  delete dummy;
#endif

  EXIT(0);
}
