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

#if __GNUC__ > 3
template<class __pool_type>
  void static_pool_instance<__pool_type>::create(void)
  {
    LIBCWD_TSD_DECLARATION;
    M_internal = __libcwd_tsd.internal;
    ptr = new __pool_type;
    if (!M_internal)
      AllocTag(ptr, "Memory pool base of libcwd::_private_::userspace_allocator, used in libcwd::_private_::string.  Will never be freed again.");
  }
#endif

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
#if LIBCWD_THREAD_SAFE
    TSD_st* tsd;
#endif
    if (internal == auto_internal_pool)
    {
#if LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION;
      tsd = &(LIBCWD_TSD);
#endif
      set_alloc_checking_off(LIBCWD_TSD);
    }
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    T* ret = (T*) M_char_allocator.allocate(num * sizeof(T));
    if (internal == auto_internal_pool)
    {
#if LIBCWD_THREAD_SAFE
      set_alloc_checking_on(*tsd);
#else
      set_alloc_checking_on(LIBCWD_TSD);
#endif
    }
    return ret;
  }

template<typename T, class CharAlloc, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  inline void
  allocator_adaptor<T, CharAlloc, internal LIBCWD_COMMA_INSTANCE>::
  deallocate(pointer p, size_t num)
  {
#if LIBCWD_THREAD_SAFE
    TSD_st* tsd;
#endif
    if (internal == auto_internal_pool)
    {
#if LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION;
      tsd = &(LIBCWD_TSD);
#endif
      set_alloc_checking_off(LIBCWD_TSD);
    }
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    M_char_allocator.deallocate((typename CharAlloc::pointer)p, num * sizeof(T));
    if (internal == auto_internal_pool)
    {
#if LIBCWD_THREAD_SAFE
      set_alloc_checking_on(*tsd);
#else
      set_alloc_checking_on(LIBCWD_TSD);
#endif
    }
  }

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

  } // namespace _private_
} // namespace libcwd
 
#endif // CWDEBUG_ALLOC
#endif // LIBCWD_PRIVATE_ALLOCATOR_INL

