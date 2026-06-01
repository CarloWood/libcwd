#ifndef PRIVATE_DEBUG_STACK_INL
#define PRIVATE_DEBUG_STACK_INL

#include "libcwd/private_debug_stack.h"
#include "libcwd/private_assert.h"
#include "libcwd/core_dump.h"
#include <cstddef>		// Needed for size_t

namespace libcwd::_private_ {

// Stack implementation that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template<typename T>
  inline
  void
  debug_stack_tst<T>::init()
  {
    p = st - 1;
    end = st + 63;
  }

template<typename T>
  inline
  void
  debug_stack_tst<T>::push(T ptr)
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    if (p == end)
      core_dump();	// This is really not normal, if you core here you probably did something wrong.
			// Doing a back trace in gdb should reveal an `infinite' debug output re-entrance loop.
			// This means that while printing debug output you call a function that makes
			// your program return to the same line, starting to print out that debug output
			// again. Try to break this loop somehow.
    *++p = ptr;
  }

extern void print_pop_error();

template<typename T>
  inline
  void
  debug_stack_tst<T>::pop()
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    if (p == st - 1)
      print_pop_error();
    --p;
  }

template<typename T>
  inline
  T
  debug_stack_tst<T>::top() const
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    return *p;
  }

template<typename T>
  inline
  size_t
  debug_stack_tst<T>::size() const
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    return p - (st - 1);
  }

}  // namespace libcwd::_private_

#endif // PRIVATE_DEBUG_STACK_INL
