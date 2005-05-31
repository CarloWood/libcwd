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

//===================================================================================================
// Allocators
//
//

/* The allocators used by libcwd have the following characteristics:

   1) The type T that is being allocated and deallocated.
   2) Whether or not the allocation is internal, auto-internal or in userspace.
   3) The pool instance from which the allocation should be drawn.
   4) Whether or not a lock is needed for this pool.
   5) Whether or not this allocation belongs to a libcwd
      critical area and if so, which one.

   Note that each critical area (if any) uses its own lock and
   therefore no (additional) lock will be needed for the allocator.
   Otherwise a lock is always needed (in the multi-threaded case).
   As of gcc 4.0, the used pool allocator doesn't use locks anymore
   but separates the pools per thread (except for one common pool),
   this need is equivalent for us to needing a lock or not: if we
   don't need a lock then there is also no need to separate per thread.

   There are five different allocators in use by libcwd:

Multi-threaded case:

   Allocator name		| internal | Pool instance         		| Needs lock
   ----------------------------------------------------------------------------------------------------
   memblk_map_allocator		| yes      | memblk_map_instance		| no (memblk_map_instance critical area)
   object_files_allocator	| yes      | object_files_instance		| no (object_files_instance critical area)
   internal_allocator		| yes      | multi_threaded_internal_instance	| yes
   auto_internal_allocator	| auto     | multi_threaded_internal_instance	| yes
   userspace_allocator		| no       | userspace_instance			| yes

Single-threaded case:

   Allocator name		| internal | Pool instance         		| Needs lock
   ----------------------------------------------------------------------------------------------------
   memblk_map_allocator		| yes      | single_threaded_internal_instance  | no
   object_files_allocator	| yes      | single_threaded_internal_instance	| no
   internal_allocator		| yes      | single_threaded_internal_instance	| no
   auto_internal_allocator	| auto     | single_threaded_internal_instance	| no
   userspace_allocator		| no       | std::alloc				| -

*/

#if __GNUC__ == 3 && __GNUC_MINOR__ == 4
#include <ext/pool_allocator.h>		// __gnu_cxx::__pool_alloc
#endif
#if __GNUC__ >= 4
#include <ext/mt_allocator.h>		// __gnu_cxx::__pool
#endif

namespace libcwd {
  namespace _private_ {

// This is a random number in the hope nobody else uses it.
int const random_salt = 327665;

// Dummy mutex instance numbers, these must be negative.
int const multi_threaded_internal_instance = -1;
int const single_threaded_internal_instance = -2;
int const userspace_instance = -3;

#if __GNUC__ == 3 && __GNUC_MINOR__ < 4
template<bool needs_lock, int pool_instance>
  struct CharPoolAlloc : public std::__default_alloc_template<needs_lock, random_salt + pool_instance> {
    typedef char* pointer;
  };
#elif __GNUC__ == 3 && __GNUC_MINOR__ == 4 && __GNUC_PATCHLEVEL__ == 0
template<bool needs_lock, int pool_instance>
  struct CharPoolAlloc : public __gnu_cxx::__pool_alloc<needs_lock, random_salt + pool_instance> {
    typedef char* pointer;
  };
#else // gcc 3.4.1 and higher.
template<int pool_instance>
  struct char_wrapper {
    char c;
  };
#if __GNUC__ == 3
// gcc 3.4.1 and 3.4.2 always use a lock, in the threaded case.
template<bool needs_lock, int pool_instance>
  class CharPoolAlloc : public __gnu_cxx::__pool_alloc<char_wrapper<pool_instance> > { };
#else // gcc 4.0 and higher.
// A wrapper around a pointer to the actual pool type (__gnu_cxx::__pool<>)
// that automatically deletes its pointer on destruction.
// This class does not need a reference counter because we only use it for static objects.
template<class __pool_type>
  struct static_pool_instance {
    __pool_type* ptr;
    bool M_internal;
    static_pool_instance(void) { }	// Do NOT initialize `ptr'! It is automatically initialized
    					// to NULL because this is a static POD object and ptr might
					// already be initialized before the global constructor of
					// this object would be called.
    void create(void);			// This does the initialization.
  };
// The class that holds the actual pool instance.
// This class also defines policies (so far only whether or not threading is used).
template<int pool_instance, bool needs_lock>
  struct pool_instance_and_policy {
    typedef __gnu_cxx::__pool<needs_lock> pool_type;		// Underlaying pool type.
    static static_pool_instance<pool_type> _S_pool_instance;	// The actual pool instance.

    // The following is needed as interface of a 'pool_policy' class as used by __gnu_cxx::__mt_alloc.
    static pool_type& _S_get_pool(void)				// Accessor to the pool_type singleton.
        { return *_S_pool_instance.ptr; }
    static void _S_initialize_once(void) 			// This is called every time a new allocation is done.
    { 
      static bool __init;
      if (__builtin_expect(__init == false, false))
      {
        _S_pool_instance.create();
	_S_pool_instance.ptr->_M_initialize_once(); 
	__init = true;
      }
    }
  };

#ifdef __GTHREADS
// Specialization, needed because in this case more interface is needed for the
// 'pool_policy' class as used by __gnu_cxx::__mt_alloc.
template<int pool_instance>
  struct pool_instance_and_policy<pool_instance, true>
  {
    typedef __gnu_cxx::__pool<true> pool_type;			// Underlaying pool type.
    static static_pool_instance<pool_type> _S_pool_instance;	// The actual pool instance.
    
    // The following is needed as interface of a 'pool_policy' class as used by __gnu_cxx::__mt_alloc.
    static pool_type& _S_get_pool(void)				// Accessor to the pool_type singleton.
        { return *_S_pool_instance.ptr; }
    static void _S_initialize_once(void) 			// This is called every time a new allocation is done.
    { 
      static bool __init;
      if (__builtin_expect(__init == false, false))
      {
        _S_pool_instance.create();
	_S_pool_instance.ptr->_M_initialize_once(_S_initialize);	// Passes _S_initialize in this case.
	__init = true;
      }
    }
    // And the extra interface:
    static void _S_destroy_thread_key(void* __freelist_pos) { _S_get_pool()._M_destroy_thread_key(__freelist_pos); }
    static void _S_initialize(void) { _S_get_pool()._M_initialize(_S_destroy_thread_key); }
  };

template<int pool_instance>
  static_pool_instance<typename pool_instance_and_policy<pool_instance, true>::pool_type>
      pool_instance_and_policy<pool_instance, true>::_S_pool_instance;
#endif // __GTHREADS

template<int pool_instance, bool needs_lock>
  static_pool_instance<typename pool_instance_and_policy<pool_instance, needs_lock>::pool_type>
      pool_instance_and_policy<pool_instance, needs_lock>::_S_pool_instance;

template<bool needs_lock, int pool_instance>
  class CharPoolAlloc : public __gnu_cxx::__mt_alloc<char, pool_instance_and_policy<pool_instance, needs_lock> > { };
#endif // gcc 4.0 and higher.
#endif // gcc 3.4.1 and higher.

// Convenience macros.
#if CWDEBUG_DEBUG
#define LIBCWD_COMMA_INT_INSTANCE , int instance
#define LIBCWD_COMMA_INSTANCE , instance
#define LIBCWD_DEBUGDEBUG_COMMA(x) , x
#else
#define LIBCWD_COMMA_INT_INSTANCE
#define LIBCWD_COMMA_INSTANCE
#define LIBCWD_DEBUGDEBUG_COMMA(x)
#endif

enum pool_nt {
  userspace_pool,
  internal_pool,
  auto_internal_pool
};

// This wrapper adds sanity checks to the allocator use (like testing if
// 'internal' allocators are indeed only used while in internal mode, and
// critical area allocators are only used when the related lock is indeed
// locked etc.
template<typename T, class CharAlloc, pool_nt internal LIBCWD_COMMA_INT_INSTANCE>
    class allocator_adaptor {
    private:
      // The underlying allocator.
      CharAlloc M_char_allocator;

    public:
      // Type definitions.
      typedef T		value_type;
      typedef size_t	size_type;
      typedef ptrdiff_t	difference_type;
      typedef T*		pointer;
      typedef T const*	const_pointer;
      typedef T&		reference;
      typedef T const&	const_reference;

      // Rebind allocator to type U.
      template <class U>
	struct rebind {
	  typedef allocator_adaptor<U, CharAlloc, internal LIBCWD_COMMA_INSTANCE> other;
	};

      // Return address of values.
      pointer address(reference value) const { return &value; }
      const_pointer address(const_reference value) const { return &value; }

      // Constructors and destructor.
      allocator_adaptor(void) throw() { }
      allocator_adaptor(allocator_adaptor const& a) : M_char_allocator(a.M_char_allocator) { }
      template<class U>
	allocator_adaptor(allocator_adaptor<U, CharAlloc, internal LIBCWD_COMMA_INSTANCE> const& a) :
	    M_char_allocator(a.M_char_allocator) { }
      template<class T2, class CharAlloc2, pool_nt internal2 LIBCWD_DEBUGDEBUG_COMMA(int instance2)>
	friend class allocator_adaptor;
      ~allocator_adaptor() throw() { }

      // Return maximum number of elements that can be allocated.
      size_type max_size(void) const { return M_char_allocator.max_size() / sizeof(T); }

      // Allocate but don't initialize num elements of type T.
      pointer allocate(size_type num);
      pointer allocate(size_type num, void const* hint);

      // Deallocate storage p of deleted elements.
      void deallocate(pointer p, size_type num);

      // Initialize elements of allocated storage p with value value.
      void construct(pointer p, T const& value) { new ((void*)p) T(value); }

      // Destroy elements of initialized storage p.
      void destroy(pointer p) { p->~T(); }

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGM
    private:
      static void sanity_check(void);
#endif

      template <class T1, class CharAlloc1, pool_nt internal1 LIBCWD_DEBUGDEBUG_COMMA(int inst1),
		class T2, class CharAlloc2, pool_nt internal2 LIBCWD_DEBUGDEBUG_COMMA(int inst2)>
	friend inline
	bool operator==(allocator_adaptor<T1, CharAlloc1, internal1 LIBCWD_DEBUGDEBUG_COMMA(inst1)> const& a1,
			allocator_adaptor<T2, CharAlloc2, internal2 LIBCWD_DEBUGDEBUG_COMMA(inst2)> const& a2);
      template <class T1, class CharAlloc1, pool_nt internal1 LIBCWD_DEBUGDEBUG_COMMA(int inst1),
		class T2, class CharAlloc2, pool_nt internal2 LIBCWD_DEBUGDEBUG_COMMA(int inst2)>
	friend inline
	bool operator!=(allocator_adaptor<T1, CharAlloc1, internal1 LIBCWD_DEBUGDEBUG_COMMA(inst1)> const& a1,
			allocator_adaptor<T2, CharAlloc2, internal2 LIBCWD_DEBUGDEBUG_COMMA(inst2)> const& a2);
    };

#if LIBCWD_THREAD_SAFE
// We normally would be able to use the default allocator, but... libcwd functions can
// at all times be called from malloc which might be called from std::allocator with its
// lock set.  Therefore we also use a separate allocator pool for the userspace, in the
// threaded case.
#define LIBCWD_CHARALLOCATOR_USERSPACE(instance) ::libcwd::_private_::				\
	allocator_adaptor<char,									\
	  		  CharPoolAlloc<true, userspace_instance>,				\
			  userspace_pool							\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcwd::_private_::instance)>
#endif

// Both, multi_threaded_internal_instance and memblk_map_instance use also locks for
// the allocator pool itself because they (the memory pools) are being shared between
// threads from within critical areas with different mutexes.
// Other instances (> 0) are supposed to only use the allocator instance from within
// the critical area of the corresponding mutex_tct<instance>, and thus only by one
// thread at a time.
#if LIBCWD_THREAD_SAFE
#define LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance)						\
			        ::libcwd::_private_::instance ==				\
			        ::libcwd::_private_::multi_threaded_internal_instance ||	\
			        ::libcwd::_private_::instance ==				\
			        ::libcwd::_private_::memblk_map_instance
#else // !LIBCWD_THREAD_SAFE
#define LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance) false
#endif // !LIBCWD_THREAD_SAFE

#define LIBCWD_CHARALLOCATOR_INTERNAL(instance) ::libcwd::_private_::			\
	allocator_adaptor<char,									\
			  CharPoolAlloc<LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance),		\
					::libcwd::_private_::instance >,			\
			  internal_pool								\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcwd::_private_::instance)>

#define LIBCWD_CHARALLOCATOR_AUTO_INTERNAL(instance) ::libcwd::_private_::		\
	allocator_adaptor<char,									\
			  CharPoolAlloc<LIBCWD_ALLOCATOR_POOL_NEEDS_LOCK(instance),		\
					::libcwd::_private_::instance >,			\
			  auto_internal_pool							\
			  LIBCWD_DEBUGDEBUG_COMMA(::libcwd::_private_::instance)>

#if LIBCWD_THREAD_SAFE
// Our allocator adaptor for the Non-Shared internal cases: Single Threaded
// (inst = single_threaded_internal_instance) or inside the critical area of the corresponding
// libcwd mutex instance.
#define LIBCWD_NS_INTERNAL_ALLOCATOR(instance)	LIBCWD_CHARALLOCATOR_INTERNAL(instance)
#else // !LIBCWD_THREAD_SAFE
// In a single threaded application, the Non-Shared case is equivalent to the Single Threaded case.
#define LIBCWD_NS_INTERNAL_ALLOCATOR(instance)	LIBCWD_CHARALLOCATOR_INTERNAL(single_threaded_internal_instance)
#endif // !LIBCWD_THREAD_SAFE

#if LIBCWD_THREAD_SAFE
// LIBCWD_MT_*_ALLOCATOR uses a different allocator than the normal default allocator of libstdc++
// in the case of multi-threading because it can be that the allocator mutex is locked, which would
// result in a deadlock if we try to use it again here.
#define LIBCWD_MT_USERSPACE_ALLOCATOR		LIBCWD_CHARALLOCATOR_USERSPACE(userspace_instance)
#define LIBCWD_MT_INTERNAL_ALLOCATOR		LIBCWD_CHARALLOCATOR_INTERNAL(multi_threaded_internal_instance)
#define LIBCWD_MT_AUTO_INTERNAL_ALLOCATOR	LIBCWD_CHARALLOCATOR_AUTO_INTERNAL(multi_threaded_internal_instance)
#else // !LIBCWD_THREAD_SAFE
// LIBCWD_MT_*_ALLOCATOR uses the normal default allocator of libstdc++-v3 (alloc) using locking
// itself.  The userspace allocator shares it memory pool with everything else (that uses this
// allocator, which is most of the (userspace) STL).
#define LIBCWD_MT_USERSPACE_ALLOCATOR		std::allocator<char>
#define LIBCWD_MT_INTERNAL_ALLOCATOR		LIBCWD_CHARALLOCATOR_INTERNAL(single_threaded_internal_instance)
#define LIBCWD_MT_AUTO_INTERNAL_ALLOCATOR	LIBCWD_CHARALLOCATOR_AUTO_INTERNAL(single_threaded_internal_instance)
#endif // !LIBCWD_THREAD_SAFE

//---------------------------------------------------------------------------------------------------
// Internal allocator types.

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

// This general allocator can be used outside libcwd-specific critical areas.
typedef LIBCWD_MT_USERSPACE_ALLOCATOR userspace_allocator;

  } // namespace _private_
} // namespace libcwd
 
#endif // CWDEBUG_ALLOC
#endif // LIBCWD_PRIVATE_ALLOCATOR_H

