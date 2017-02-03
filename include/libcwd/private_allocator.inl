// $Header$
//
// Copyright (C) 2001 - 2004, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_PRIVATE_ALLOCATOR_INL
#define LIBCWD_PRIVATE_ALLOCATOR_INL

#if CWDEBUG_ALLOC

#ifndef LIBCWD_PRIVATE_THREAD_H
#include <libcwd/private_thread.h>
#endif
#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#include <libcwd/private_struct_TSD.h>
#endif
#ifndef LIBCWD_MACRO_ALLOCTAG_H
#include <libcwd/macro_AllocTag.h>
#endif

namespace libcwd {
  namespace _private_ {

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
template<typename T, class CharAlloc, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  void
  allocator_adaptor<T, CharAlloc, internal LIBCWD_COMMA_INSTANCE>::sanity_check(void)
  {
#if CWDEBUG_DEBUGM || (CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE)
    LIBCWD_TSD_DECLARATION;
#endif
#if CWDEBUG_DEBUGM
    if ((__libcwd_tsd.internal > 0) != (internal != userspace_pool))
      core_dump();
#endif
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
    if (WST_multi_threaded)
    {
      if (instance == single_threaded_internal_instance)
	core_dump();
      // Check if the correct lock is set.
      if (instance == memblk_map_instance)
      {
	if (!__libcwd_tsd.target_thread->thread_mutex.is_locked())
	  core_dump();
      }
      else if (instance >= 0 && !is_locked(instance))
	core_dump();
    }
#endif
  }
#endif

template<typename T, class CharAlloc, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  inline T*
  allocator_adaptor<T, CharAlloc, internal LIBCWD_COMMA_INSTANCE>::allocate(size_t num)
  {
    LIBCWD_TSD_DECLARATION;
    if (internal == auto_internal_pool)
      set_alloc_checking_off(LIBCWD_TSD);
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    T* ret = (T*) M_char_allocator.allocate(num * sizeof(T)
#if __GNUC__ >= 4
        LIBCWD_COMMA_TSD
#endif
	);
    if (internal == auto_internal_pool)
      set_alloc_checking_on(LIBCWD_TSD);
    return ret;
  }

template<typename T, class CharAlloc, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  inline void
  allocator_adaptor<T, CharAlloc, internal LIBCWD_COMMA_INSTANCE>::
  deallocate(pointer p, size_t num)
  {
    LIBCWD_TSD_DECLARATION;
    if (internal == auto_internal_pool)
      set_alloc_checking_off(LIBCWD_TSD);
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    M_char_allocator.deallocate((typename CharAlloc::pointer)p, num * sizeof(T)
#if __GNUC__ >= 4
        LIBCWD_COMMA_TSD
#endif
        );
    if (internal == auto_internal_pool)
      set_alloc_checking_on(LIBCWD_TSD);
  }

#if __GNUC__ >= 4
template <bool needs_lock1, int pool_instance1,
	  bool needs_lock2, int pool_instance2>
  inline bool
  operator==(CharPoolAlloc<needs_lock1, pool_instance1> const&,
	     CharPoolAlloc<needs_lock2, pool_instance2> const&)
  {
    return needs_lock1 == needs_lock2 && pool_instance1 == pool_instance2;
  }

template <bool needs_lock1, int pool_instance1,
	  bool needs_lock2, int pool_instance2>
  inline bool
  operator!=(CharPoolAlloc<needs_lock1, pool_instance1> const&,
  	     CharPoolAlloc<needs_lock2, pool_instance2> const&)
  {
    return needs_lock1 != needs_lock2 || pool_instance1 != pool_instance2;
  }
#endif

template <typename T1, class CharAlloc1, pool_nt internal1 LIBCWD_DEBUGDEBUG_COMMA(int inst1),
	  typename T2, class CharAlloc2, pool_nt internal2 LIBCWD_DEBUGDEBUG_COMMA(int inst2)>
  inline bool
  operator==(allocator_adaptor<T1, CharAlloc1, internal1 LIBCWD_DEBUGDEBUG_COMMA(inst1)> const& a1,
	     allocator_adaptor<T2, CharAlloc2, internal2 LIBCWD_DEBUGDEBUG_COMMA(inst2)> const& a2)
  {
    return
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 2)
	// At least g++ 3.2.3 crashes on this.  Lets just hope that they are always equal (as they should).
        internal1 == internal2 &&
#if CWDEBUG_DEBUG
	inst1 == inst2 &&
#endif
#endif
	a1.M_char_allocator == a2.M_char_allocator;
  }

template <typename T1, class CharAlloc1, pool_nt internal1 LIBCWD_DEBUGDEBUG_COMMA(int inst1),
	  typename T2, class CharAlloc2, pool_nt internal2 LIBCWD_DEBUGDEBUG_COMMA(int inst2)>
  inline bool
  operator!=(allocator_adaptor<T1, CharAlloc1, internal1 LIBCWD_DEBUGDEBUG_COMMA(inst1)> const& a1,
	     allocator_adaptor<T2, CharAlloc2, internal2 LIBCWD_DEBUGDEBUG_COMMA(inst2)> const& a2)
  {
    return (internal1 != internal2 ||
#if CWDEBUG_DEBUG
	    inst1 != inst2 ||
#endif
	    a1.M_char_allocator != a2.M_char_allocator);
  }


#if __GNUC__ >= 4
// Find the most significant bit in an unsigned long.
// This function is fastest for small values of 'val',
// but 'val' should be larger than 15 (otherwise 4
// is returned regardless).
inline int find1(unsigned long val)
{
  unsigned long mask = ~(minimum_size - 1);	// ...1111100000
  int bit = minimum_size_exp - 1;		// bit: 76543210
  while ((mask & val))
  {
    mask <<= 1;
    ++bit;
  }
  return bit;
}

template <bool needs_lock, int pool_instance>
  FreeList CharPoolAlloc<needs_lock, pool_instance>::S_freelist;

template <bool needs_lock, int pool_instance>
  char* CharPoolAlloc<needs_lock, pool_instance>::allocate(size_type size LIBCWD_COMMA_TSD_PARAM)
  {
    size += sizeof(void*);				// We'll add a pointer at the start of the returned memory block.
    int power = find1((unsigned long)size - 1) + 1;	// Round up to first power of 2;
    // power >= minimum_size_exp
    size = (1U << power);				// Is at least minimum_size.
    if (size > maximum_size)				// No free lists for large chunks.
      return (char*)::operator new(size - sizeof(void*));
    if (!S_freelist.M_initialized)
      S_freelist.initialize(LIBCWD_TSD);
    char* ptr;
#if LIBCWD_THREAD_SAFE
    if (needs_lock)
    {
      LIBCWD_DEFER_CANCEL_NO_BRACE;
      pthread_mutex_lock(&S_freelist.M_mutex);
      ptr = S_freelist.allocate(power, size);
      pthread_mutex_unlock(&S_freelist.M_mutex);
      int internal_saved = __libcwd_tsd.internal;
      __libcwd_tsd.internal = 0;
      LIBCWD_RESTORE_CANCEL_NO_BRACE;
      __libcwd_tsd.internal = internal_saved;
    }
    else
#endif
      ptr = S_freelist.allocate(power, size);
    return ptr;
  }

template <bool needs_lock, int pool_instance>
  void CharPoolAlloc<needs_lock, pool_instance>::deallocate(pointer p, size_type size LIBCWD_COMMA_TSD_PARAM)
  {
    size += sizeof(void*);
    int power = find1((unsigned long)size - 1) + 1;
    size = (1U << power);
    if (size > maximum_size)		// No free lists for large chunks.
    {
      ::operator delete(p);
      return;
    }
#if LIBCWD_THREAD_SAFE
    if (needs_lock)
    {
      LIBCWD_DEFER_CANCEL_NO_BRACE;
      pthread_mutex_lock(&S_freelist.M_mutex);
      S_freelist.deallocate(p, power, size);
      pthread_mutex_unlock(&S_freelist.M_mutex);
      int internal_saved = __libcwd_tsd.internal;
      __libcwd_tsd.internal = 0;
      LIBCWD_RESTORE_CANCEL_NO_BRACE;
      __libcwd_tsd.internal = internal_saved;
    }
    else
#endif
      S_freelist.deallocate(p, power, size);
  }
#endif

  } // namespace _private_
} // namespace libcwd
 
#endif // CWDEBUG_ALLOC
#endif // LIBCWD_PRIVATE_ALLOCATOR_INL

