// $Header$
//
// Copyright (C) 2001 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_PRIVATE_ALLOCATOR_INL
#define LIBCW_PRIVATE_ALLOCATOR_INL

#if CWDEBUG_ALLOC

#ifndef LIBCW_PRIVATE_THREAD_H
#include <libcw/private_thread.h>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
template<class T, class X, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  void allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::sanity_check(void)
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

template<class T, class X, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  T*
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::allocate(size_t n)
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
    T* ret = (T*) X::allocate(n * sizeof(T));
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

template<class T, class X, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  T*
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::allocate(void)
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
    T* ret = (T*) X::allocate(sizeof(T));
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

template<class T, class X, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  void
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::
  deallocate(deallocate_pointer p, size_t n)
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
    X::deallocate(p, n * sizeof(T));
    if (internal == auto_internal_pool)
    {
#if LIBCWD_THREAD_SAFE
      set_alloc_checking_on(*tsd);
#else
      set_alloc_checking_on(LIBCWD_TSD);
#endif
    }
  }

template<class T, class X, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  void
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::deallocate(deallocate_pointer p)
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
    X::deallocate(p, sizeof(T));
    if (internal == auto_internal_pool)
    {
#if LIBCWD_THREAD_SAFE
      set_alloc_checking_on(*tsd);
#else
      set_alloc_checking_on(LIBCWD_TSD);
#endif
    }
  }

#if !CWDEBUG_DEBUG
template <class T1, class X1, pool_nt internal1,
          class T2, class X2, pool_nt internal2>
  __inline__
  bool
  operator==(allocator_adaptor<T1, X1, internal1> const& a1,
             allocator_adaptor<T2, X2, internal2> const& a2)
  {
    return (internal1 == internal2 && a1.M_underlying_alloc == a2.M_underlying_alloc);
  }
 
template <class T1, class X1, pool_nt internal1,
          class T2, class X2, pool_nt internal2>
  __inline__
  bool
  operator!=(allocator_adaptor<T1, X1, internal1> const& a1,
             allocator_adaptor<T2, X2, internal2> const& a2)
  {
    return (internal1 != internal2 || a1.M_underlying_alloc != a2.M_underlying_alloc);
  }
#else
template <class T1, class X1, pool_nt internal1, int inst1,
          class T2, class X2, pool_nt internal2, int inst2>
  __inline__
  bool
  operator==(allocator_adaptor<T1, X1, internal1, inst1> const& a1,
             allocator_adaptor<T2, X2, internal2, inst2> const& a2)
  {
    return (internal1 == internal2 && inst1 == inst2
	    && a1.M_underlying_alloc == a2.M_underlying_alloc);
  }
 
template <class T1, class X1, pool_nt internal1, int inst1,
          class T2, class X2, pool_nt internal2, int inst2>
  __inline__
  bool
  operator!=(allocator_adaptor<T1, X1, internal1, inst1> const& a1,
             allocator_adaptor<T2, X2, internal2, inst2> const& a2)
  {
    return (internal1 != internal2 || inst1 != inst2
	    || a1.M_underlying_alloc != a2.M_underlying_alloc);
  }
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#endif // CWDEBUG_ALLOC
#endif // LIBCW_PRIVATE_ALLOCATOR_INL

