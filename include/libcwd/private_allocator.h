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

/** \file libcwd/private_allocator.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_PRIVATE_ALLOCATOR_H
#define LIBCWD_PRIVATE_ALLOCATOR_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif

#if CWDEBUG_ALLOC		// This file is not used when --disable-alloc was used.

#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#include <libcwd/private_mutex_instances.h>
#endif
#ifndef LIBCWD_CORE_DUMP_H
#include <libcwd/core_dump.h>
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

#if __GNUC_MINOR__ < 4
#define LIBCWD_POOL_ALLOC std::__default_alloc_template
#else
#include <ext/pool_allocator.h>	// If you don't have this, upgrade your gcc 3.4 from CVS.  FIXME: remove comment after release of 3.4
#define LIBCWD_POOL_ALLOC __gnu_cxx::__pool_alloc
#endif

// This is a random number in the hope nobody else uses it.
int const random_salt = 327665;
// Dummy mutex instance numbers, these must be negative.
int const multi_threaded_internal_instance = -1;
int const multi_threaded_userspace_instance = -random_salt;		// Use LIBCWD_POOL_ALLOC<true, 0>
int const single_threaded_internal_instance = -2;
int const single_threaded_userspace_instance = multi_threaded_userspace_instance;

// An allocator that doesn't use the same lock as LIBCWD_POOL_ALLOC<true, 0>
// We normally would be able to use the default allocator, but... libcwd functions can
// at all times be called from malloc which might be called from LIBCWD_POOL_ALLOC<true, 0>
// with its lock set.
typedef LIBCWD_POOL_ALLOC<true, random_salt - 3> our_userspace_allocator;

#if CWDEBUG_DEBUG
#define LIBCWD_COMMA_INT_INSTANCE , int instance
#define LIBCWD_COMMA_INSTANCE , instance
#else
#define LIBCWD_COMMA_INT_INSTANCE
#define LIBCWD_COMMA_INSTANCE
#endif

enum pool_nt {
  userspace_pool,
  internal_pool,
  auto_internal_pool
};

template<class T, class X, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
  struct allocator_adaptor {
    typedef T*		pointer;
    typedef T const*	const_pointer;
    typedef T&		reference;
    typedef T const&	const_reference;
    typedef T		value_type;
    typedef size_t	size_type;
    typedef ptrdiff_t	difference_type;
    typedef T*		deallocate_pointer;

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

    allocator_adaptor(void) { }
    allocator_adaptor(allocator_adaptor const& a) :
        M_underlying_alloc(a.M_underlying_alloc) { }
    template <class T1>
      allocator_adaptor(allocator_adaptor<T1, X, internal LIBCWD_COMMA_INSTANCE> const& a) :
          M_underlying_alloc(a.M_underlying_alloc) { }
    ~allocator_adaptor() { }

    size_type max_size(void) const { return size_t(-1) / sizeof(T); }
       
    void construct(pointer p, const_reference t) { new((void*)p) T(t); }
    void destroy(pointer p) { p->~T(); }

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
  private:
    static void sanity_check(void);
#endif
  };

#if CWDEBUG_DEBUG
#define LIBCWD_DEBUGDEBUG_COMMA(x) , x
#else
#define LIBCWD_DEBUGDEBUG_COMMA(x)
#endif

// LIBCWD_POOL_ALLOC<true, 0> is the default thread-safe STL allocator std::__alloc,
// but we use the longer type because in gcc 3.0.x the underscores were missing and it would be
// std::alloc.  See also http://gcc.gnu.org/onlinedocs/libstdc++/ext/howto.html#3.
#define LIBCWD_DEFAULT_ALLOC_USERSPACE(instance) ::libcw::debug::_private_::			\
	allocator_adaptor<char,									\
	  		  our_userspace_allocator,						\
			  userspace_pool							\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcw::debug::_private_::instance)>

// Both, multi_threaded_internal_instance and memblk_map_instance use also locks for
// the allocator pool itself because they (the memory pools) are being shared between
// threads from within critical areas with different mutexes.
// Other instances (> 0) are supposed to only use the allocator instance from within
// the critical area of the corresponding mutex_tct<instance>, and thus only by one
// thread at a time.
#if LIBCWD_THREAD_SAFE
#define LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance)						\
			        ::libcw::debug::_private_::instance ==				\
			        ::libcw::debug::_private_::multi_threaded_internal_instance ||	\
			        ::libcw::debug::_private_::instance ==				\
			        ::libcw::debug::_private_::memblk_map_instance
#else // !LIBCWD_THREAD_SAFE
#define LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance) false
#endif // !LIBCWD_THREAD_SAFE

#define LIBCWD_DEFAULT_ALLOC_INTERNAL(instance) ::libcw::debug::_private_::			\
	allocator_adaptor<char,									\
			  LIBCWD_POOL_ALLOC							\
			      <LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance),			\
			        ::libcw::debug::_private_::random_salt +			\
				::libcw::debug::_private_::instance >,				\
			  internal_pool								\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcw::debug::_private_::instance)>

#define LIBCWD_DEFAULT_ALLOC_AUTO_INTERNAL(instance) ::libcw::debug::_private_::		\
	allocator_adaptor<char,									\
			  LIBCWD_POOL_ALLOC							\
			      <LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance),			\
			        ::libcw::debug::_private_::random_salt +			\
				::libcw::debug::_private_::instance >,				\
			  auto_internal_pool							\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcw::debug::_private_::instance)>

#if LIBCWD_THREAD_SAFE
// Our allocator adaptor for the Non-Shared internal cases: Single Threaded
// (inst = single_threaded_internal_instance) or inside the critical area of the corresponding
// libcwd mutex instance.  Using a macro here instead of another template in order not to bloat the
// mangling TOO much.
#define LIBCWD_NS_INTERNAL_ALLOCATOR(instance)	\
			  		LIBCWD_DEFAULT_ALLOC_INTERNAL(instance)
#else // !LIBCWD_THREAD_SAFE
// In a single threaded application, the Non_shared case is equivalent to the Single Threaded case.
#define LIBCWD_NS_INTERNAL_ALLOCATOR(instance) \
			  		LIBCWD_DEFAULT_ALLOC_INTERNAL(single_threaded_internal_instance)
#endif // !LIBCWD_THREAD_SAFE
// The Non-Shared userspace allocator is the same whether libcwd is thread-safe or not.
#define LIBCWD_NS_USERSPACE_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_USERSPACE(single_threaded_userspace_instance)

#if LIBCWD_THREAD_SAFE
// LIBCWD_MT_*_ALLOCATOR uses a different allocator than the normal default allocator of libstdc++
// in the case of multi-threading because it can be that the allocator mutex is locked, which would
// result in a deadlock if we try to use it again here.
#define LIBCWD_MT_USERSPACE_ALLOCATOR		LIBCWD_DEFAULT_ALLOC_USERSPACE(multi_threaded_userspace_instance)
#define LIBCWD_MT_INTERNAL_ALLOCATOR		LIBCWD_DEFAULT_ALLOC_INTERNAL(multi_threaded_internal_instance)
#define LIBCWD_MT_AUTO_INTERNAL_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_AUTO_INTERNAL(multi_threaded_internal_instance)
#else // !LIBCWD_THREAD_SAFE
// LIBCWD_MT_*_ALLOCATOR uses the normal default allocator of libstdc++-v3 (alloc) using locking
// itself.  The userspace allocator shares it memory pool with everything else (that uses this
// allocator, which is most of the (userspace) STL).
#define LIBCWD_MT_USERSPACE_ALLOCATOR		LIBCWD_DEFAULT_ALLOC_USERSPACE(single_threaded_userspace_instance)
#define LIBCWD_MT_INTERNAL_ALLOCATOR		LIBCWD_DEFAULT_ALLOC_INTERNAL(single_threaded_internal_instance)
#define LIBCWD_MT_AUTO_INTERNAL_ALLOCATOR	LIBCWD_DEFAULT_ALLOC_AUTO_INTERNAL(single_threaded_internal_instance)
#endif // !LIBCWD_THREAD_SAFE

//---------------------------------------------------------------------------------------------------
// Internal allocator types.

// In Single Threaded functions (ST_* functions) we can use an allocator type that doesn't need
// locking.
typedef LIBCWD_NS_INTERNAL_ALLOCATOR(single_threaded_internal_instance) ST_internal_allocator;

// This allocator is used in critical areas that are already locked by memblk_map_instance.
typedef LIBCWD_NS_INTERNAL_ALLOCATOR(memblk_map_instance) memblk_map_allocator;

// This allocator is used in critical areas that are already locked by object_files_instance.
typedef LIBCWD_NS_INTERNAL_ALLOCATOR(object_files_instance) object_files_allocator;

// This general allocator can be used outside libcwd-specific critical areas,
// but inside a set_alloc_checking_off() .. set_alloc_checking_on() pair.
typedef LIBCWD_MT_INTERNAL_ALLOCATOR internal_allocator;

// This general allocator can be used outside libcwd-specific critical areas,
// in "user space" but that will cause internal memory to be allocated.
typedef LIBCWD_MT_AUTO_INTERNAL_ALLOCATOR auto_internal_allocator;

//---------------------------------------------------------------------------------------------------
// User space allocator type.

// This allocator can be used outside libcwd-specific critical areas but inside Single Threaded
// functions.
typedef LIBCWD_NS_USERSPACE_ALLOCATOR ST_userspace_allocator;

// This general allocator can be used outside libcwd-specific critical areas.
typedef LIBCWD_MT_USERSPACE_ALLOCATOR userspace_allocator;

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#endif // CWDEBUG_ALLOC
#endif // LIBCWD_PRIVATE_ALLOCATOR_H

