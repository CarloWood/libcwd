#define private public
#include <bits/stl_alloc.h>
#undef private

#include "sys.h"
#include "debug.h"
#include <iostream>
#include <sstream>

size_t const _MAX_BYTES = std::alloc::_MAX_BYTES;

using libcw::debug::_private_::allocator_adaptor;

int main(void)
{
  Debug( check_configuration() );
#ifdef DEBUGMALLOC
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( dc::bfd.off() );
  Debug( libcw_do.on() );
  Debug( list_channels_on(libcw_do) );
  Debug( libcw_do.set_ostream(&std::cout) );

  std::basic_stringstream<char, std::char_traits<char>, LIBCWD_MT_USERSPACE_ALLOCATOR> debug_stream;

  std::alloc a;
  size_t size = _MAX_BYTES;

  if (size <= _MAX_BYTES)			// Correct `size' to reflect the real ammount of memory that will be allocated.
    size = std::alloc::_S_round_up(size);

  Dout(dc::notice, "_MAX_BYTES == " << _MAX_BYTES);
  Dout(dc::notice, "Allocated size == " << size);

  int const n = 2000000;
  void** ptr = new void* [n];

  Dout(dc::notice, "Allocating " << n << " times " << size << " bytes using std::alloc::allocate():");
  Debug( libcw_do.set_ostream(&debug_stream) );
  for (int i = 0; i < n; ++i)
    ptr[i] = a.allocate(size);

  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( list_allocations_on(libcw_do) );

  Dout(dc::notice, "Deallocating these " << n << " blocks using std::alloc::deallocate():");
  Debug( libcw_do.set_ostream(&debug_stream) );
  for (int i = 0; i < n; ++i)
    a.deallocate(ptr[i], size);

  Debug( libcw_do.set_ostream(&std::cout) );
  delete [] ptr;
  Debug( list_allocations_on(libcw_do) );

  return 0;
}
