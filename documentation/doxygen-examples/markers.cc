#include <libcw/sysd.h>
#include <libcw/debug.h>

// A dummy class
class A {
  int i;
  int j;
  char k;
};

int main(int argc, char* argv[])
{
  // Don't show allocations that are allocated before main()
  Debug( make_all_allocations_invisible_except(NULL) );

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
  A* a1 = new A;
  AllocTag(a1, "First created");

#ifdef DEBUGMARKER
  // Create marker
  libcw::debug::marker_ct* marker = new libcw::debug::marker_ct("A test marker");
#endif

  // Allocate more objects
  A* a2 = new A[10];
  AllocTag(a2, "Created after the marker");
  int* p = new int[30];
  AllocTag(p, "Created after the marker");

  // Show Memory Allocation Overview
  Debug( list_allocations_on(libcw_do) );

  Dout(dc::notice, "Moving the int array outside of the marker...");
#ifdef DEBUGMARKER
  Debug( move_outside(marker, p) );
#endif

  // Show Memory Allocation Overview
  Debug( list_allocations_on(libcw_do) );

#ifdef DEBUGMARKER
  // Delete the marker
  delete marker;
#endif

#ifdef DEBUGMALLOC
  Dout(dc::notice, "Finished successfully.");
#else
  DoutFatal(dc::fatal, "Please define DEBUGMALLOC.");
#endif
  return 0;
}


