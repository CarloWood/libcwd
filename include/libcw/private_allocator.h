// $Header$
//
// Copyright (C) 2001 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/private_allocator.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_ALLOCATOR_H
#define LIBCW_PRIVATE_ALLOCATOR_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif

#if CWDEBUG_ALLOC		// This file is not used unless --enable-alloc was used.

#ifndef LIBCW_PRIVATE_THREADING_H
#include <libcw/private_threading.h>
#endif
#ifndef LIBCW_CORE_DUMP_H
#include <libcw/core_dump.h>
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

//===================================================================================================
// Allocators
//
//
// This is a random number in the hope nobody else uses it.
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
// gdb-5.0 and 5.1 core dump on old-ABI mangled names with integer template parameters larger
// than one digit.  See http://sources.redhat.com/ml/bug-binutils/2002-q1/msg00021.html and
// http://sources.redhat.com/ml/bug-binutils/2002-q1/msg00023.html.
int const random_salt = 6;
#else
int const random_salt = 327665;
#endif
// Dummy mutex instance numbers, these must be negative.
int const single_threaded_userspace_instance = -random_salt;	// Use std::alloc
int const single_threaded_internal_instance = -1;
int const multi_threaded_userspace_instance = -random_salt;	// Use std::alloc
int const multi_threaded_internal_instance = -2;

#if CWDEBUG_DEBUG
#define LIBCWD_COMMA_INT_INSTANCE , int instance
#define LIBCWD_COMMA_INSTANCE , instance
#else
#define LIBCWD_COMMA_INT_INSTANCE
#define LIBCWD_COMMA_INSTANCE
#endif

template<class T, class X, bool internal LIBCWD_COMMA_INT_INSTANCE>
  struct allocator_adaptor {
    typedef T*		pointer;
    typedef T const*	const_pointer;
    typedef T&		reference;
    typedef T const&	const_reference;
    typedef T		value_type;
    typedef size_t	size_type;
    typedef ptrdiff_t	difference_type;
#if __GNUC__ < 3
    typedef void*	deallocate_pointer;
#else
    typedef T*		deallocate_pointer;
#endif

    template <class T1> struct rebind {
      typedef allocator_adaptor<T1, X, internal LIBCWD_COMMA_INSTANCE> other;
    };

    X M_underlying_alloc;

    pointer address(reference r) const { return &r; }
    const_pointer address(const_reference r) const { return &r; }

    static T* allocate(size_t n);
    static T* allocate(void);
    static void deallocate(deallocate_pointer p, size_t n);
    static void deallocate(deallocate_pointer p);

    allocator_adaptor(void) throw() { }
    allocator_adaptor(allocator_adaptor const& a) throw() :
        M_underlying_alloc(a.M_underlying_alloc) { }
    template <class T1>
      allocator_adaptor(allocator_adaptor<T1, X, internal LIBCWD_COMMA_INSTANCE> const& a) throw() :
          M_underlying_alloc(a.M_underlying_alloc) { }
    ~allocator_adaptor() throw() { }

    size_type max_size(void) const throw() { return size_t(-1) / sizeof(T); }
       
    void construct(pointer p, const_reference t) { new((void*)p) T(t); }
    void destroy(pointer p) { p->~T(); }

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
  private:
    static void sanity_check(void);
#endif
  };

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
template<class T, class X, bool internal LIBCWD_COMMA_INT_INSTANCE>
  void allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::sanity_check(void)
  {
#if CWDEBUG_DEBUGM
    LIBCWD_TSD_DECLARATION
    if ((__libcwd_tsd.internal > 0) != internal) 
      core_dump();
#endif
#if CWDEBUG_DEBUG && defined(_REENTRANT)
    if (instance == single_threaded_internal_instance && WST_multi_threaded)
      core_dump();
    if (instance >= 0 && WST_multi_threaded && !is_locked(instance))
      core_dump();
#endif
  }
#endif

template<class T, class X, bool internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  T*
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::allocate(size_t n)
  {
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    return (T*) X::allocate(n * sizeof(T));
  }

template<class T, class X, bool internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  T*
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::allocate(void)
  {
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    return (T*) X::allocate(sizeof(T));
  }

template<class T, class X, bool internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  void
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::
  deallocate(deallocate_pointer p, size_t n)
  {
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    X::deallocate(p, n * sizeof(T));
  }

template<class T, class X, bool internal LIBCWD_COMMA_INT_INSTANCE>
  __inline__
  void
  allocator_adaptor<T, X, internal LIBCWD_COMMA_INSTANCE>::deallocate(deallocate_pointer p)
  {
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    sanity_check();
#endif
    X::deallocate(p, sizeof(T));
  }

#if !CWDEBUG_DEBUG
template <class T1, class X1, bool internal1,
          class T2, class X2, bool internal2>
  __inline__
  bool
  operator==(allocator_adaptor<T1, X1, internal1> const& a1,
             allocator_adaptor<T2, X2, internal2> const& a2)
  {
    return (internal1 == internal2 && a1.M_underlying_alloc == a2.M_underlying_alloc);
  }
 
template <class T1, class X1, bool internal1,
          class T2, class X2, bool internal2>
  __inline__
  bool
  operator!=(allocator_adaptor<T1, X1, internal1> const& a1,
             allocator_adaptor<T2, X2, internal2> const& a2)
  {
    return (internal1 != internal2 || a1.M_underlying_alloc != a2.M_underlying_alloc);
  }
#else
template <class T1, class X1, bool internal1, int inst1,
          class T2, class X2, bool internal2, int inst2>
  __inline__
  bool
  operator==(allocator_adaptor<T1, X1, internal1, inst1> const& a1,
             allocator_adaptor<T2, X2, internal2, inst2> const& a2)
  {
    return (internal1 == internal2 && inst1 == inst2
	    && a1.M_underlying_alloc == a2.M_underlying_alloc);
  }
 
template <class T1, class X1, bool internal1, int inst1,
          class T2, class X2, bool internal2, int inst2>
  __inline__
  bool
  operator!=(allocator_adaptor<T1, X1, internal1, inst1> const& a1,
             allocator_adaptor<T2, X2, internal2, inst2> const& a2)
  {
    return (internal1 != internal2 || inst1 != inst2
	    || a1.M_underlying_alloc != a2.M_underlying_alloc);
  }
#endif

#if CWDEBUG_DEBUG
#define LIBCWD_DEBUGDEBUG_COMMA(x) , x
#else
#define LIBCWD_DEBUGDEBUG_COMMA(x)
#endif

#define LIBCWD_DEFAULT_ALLOC_USERSPACE(instance) ::libcw::debug::_private_::	\
	allocator_adaptor<char,						\
	  		  ::std::alloc,					\
			  false						\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcw::debug::_private_::instance)>

#define LIBCWD_DEFAULT_ALLOC_INTERNAL(instance) ::libcw::debug::_private_::				\
	allocator_adaptor<char,									\
			  ::std::__default_alloc_template					\
			      < ::libcw::debug::_private_::instance ==				\
			        ::libcw::debug::_private_::multi_threaded_internal_instance,	\
			        ::libcw::debug::_private_::random_salt +			\
				::libcw::debug::_private_::instance >,				\
			  true									\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcw::debug::_private_::instance)>

#ifdef _REENTRANT
// Our allocator adaptor for the Non-Shared internal cases: Single Threaded
// (inst = single_threaded_internal_instance) or inside the critical area of the corresponding
// libcwd mutex instance.  Using a macro here instead of another template in order not to bloat the
// mangling TOO much.
#define LIBCWD_NS_INTERNAL_ALLOCATOR(instance)	\
			  		LIBCWD_DEFAULT_ALLOC_INTERNAL(instance)
#else // !_REENTRANT
// In a single threaded application, the Non_shared case is equivalent to the Single Threaded case.
#define LIBCWD_NS_INTERNAL_ALLOCATOR(instance) \
			  		LIBCWD_DEFAULT_ALLOC_INTERNAL(single_threaded_internal_instance)
#endif // !_REENTRANT

#ifdef _REENTRANT
// LIBCWD_MT_*_ALLOCATOR uses an different allocator than the normal default allocator of libstdc++
// in the case of multi-threading because it can be that the allocator mutex is locked, which would
// result in a deadlock if we try to use it again here.
#define LIBCWD_MT_INTERNAL_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_INTERNAL(multi_threaded_internal_instance)
#define LIBCWD_MT_USERSPACE_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_USERSPACE(multi_threaded_userspace_instance)
#else // !_REENTRANT
// LIBCWD_MT_*_ALLOCATOR uses the normal default allocator of libstdc++-v3 (alloc) using locking
// itself.  The userspace allocator shares it memory pool with everything else (that uses this
// allocator, which is most of the (userspace) STL).
#define LIBCWD_MT_INTERNAL_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_INTERNAL(single_threaded_internal_instance)
#define LIBCWD_MT_USERSPACE_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_USERSPACE(single_threaded_userspace_instance)
#endif // !_REENTRANT

//---------------------------------------------------------------------------------------------------
// Internal allocator types.

// In Single Threaded functions (ST_* functions) we can use an allocator type that doesn't need
// locking.
typedef LIBCWD_NS_INTERNAL_ALLOCATOR(single_threaded_internal_instance) ST_internal_allocator;

// This allocator is used in critical areas that are already locked by memblk_map_instance.
typedef LIBCWD_NS_INTERNAL_ALLOCATOR(memblk_map_instance) memblk_map_allocator;

// This allocator is used in critical areas that are already locked by object_files_instance.
typedef LIBCWD_NS_INTERNAL_ALLOCATOR(object_files_instance) object_files_allocator;

// This general allocator can be used outside libcwd-specific critical areas.
typedef LIBCWD_MT_INTERNAL_ALLOCATOR internal_allocator;

//---------------------------------------------------------------------------------------------------
// User space allocator type.

// This general allocator can be used outside libcwd-specific critical areas.
typedef LIBCWD_MT_USERSPACE_ALLOCATOR userspace_allocator;

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#endif // CWDEBUG_ALLOC
#endif // LIBCW_PRIVATE_ALLOCATOR_H

