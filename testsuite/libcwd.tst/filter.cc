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
#include <cstdlib>
#include <iostream>

MAIN_FUNCTION
{ PREFIX_CODE

  Debug( check_configuration() );

#if CWDEBUG_LOCATION
  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)exit) );
#endif

#if CWDEBUG_ALLOC && !defined(THREADTEST)
  // Don't show allocations that are allocated before main()
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

  // Select channels
  ForAllDebugChannels( while (debugChannel.is_on()) debugChannel.off(); );
  Debug( dc::malloc.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() );

  std::vector<int>* p = new std::vector<int>;
  AllocTag(p, "filter.cc");
  for(int i = 0; i < 1000; ++i)
    p->push_back(3);

  Debug(
    ooam_filter_ct filter(show_objectfile|show_path);
    std::vector<std::string> masks;
    masks.push_back("/usr/include/*");
    filter.hide_sourcefiles_matching(masks);
    list_allocations_on(libcw_do, filter)
  );

  Debug(
    ooam_filter_ct filter(show_objectfile|show_path);
    list_allocations_on(libcw_do, filter)
  );

  delete p;

  Debug( libcw_do.off() );

  EXIT(0);
}
