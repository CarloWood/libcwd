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

#include "libcw/sys.h"
#include <iostream>
#include "libcw/h.h"
#include "libcw/debug.h"

// A dummy class
class A {
  int i;
  int j;
  char k;
};

int main(int argc, char *argv[])
{
#ifdef DEBUGMALLOC
  // Don't show allocations that are allocated before main()
  make_all_allocations_invisible_except(NULL);

  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
  Debug( dc::notice.on() );
  Debug( dc::malloc.on() );
  Debug( dc::warning.on() );
  // Debug( dc::bfd.on() );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on
  Debug( libcw_do.on() ); 

  // Allocate new object
  A *a1 = new A;
  AllocTag(a1, "First created");

  // Create marker
  debugmalloc_marker_ct *marker = new debugmalloc_marker_ct("A test marker");

  // Allocate more objects
  A *a2 = new A[10];
  AllocTag(a2, "Created after the marker");
  int *p = new int[30];
  AllocTag(p, "Created after the marker");

  // Show Memory Allocation Overview
  Debug( list_allocations_on(libcw_do) );

  Dout( dc::notice, "Moving the int array outside of the marker..." );
  debug_move_outside(marker, p);

  // Show Memory Allocation Overview
  Debug( list_allocations_on(libcw_do) );

  // Delete the marker
  delete marker;

  Dout( dc::notice, "Finished successfully." );

#else // DEBUGMALLOC
  cerr << "Define DEBUGMALLOC in libcw/debugging_defs.h\n";
#endif // !DEBUGMALLOC

  return 0;
}
