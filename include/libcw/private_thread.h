// $Header$
//
// Copyright (C) 2002, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcw/private_thread.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_THREAD_H
#define LIBCW_PRIVATE_THREAD_H

#ifndef LIBCW_PRIVATE_MUTEX_H
#include <libcw/private_mutex.h>
#endif
#ifndef LIBCW_PRIVATE_ALLOCATOR_H
#include <libcw/private_allocator.h>
#endif
#ifndef LIBCW_LIST
#define LIBCW_LIST
#include <list>
#endif

namespace libcw {
  namespace debug {

class dm_alloc_ct;

    namespace _private_ {

struct TSD_st;

// class thread_ct
//
// Each created thread gets an object of type `thread_ct' assigned.
// The objects are stored in an STL list so that we can use pointers
// to the objects without the fear that these pointers become invalid.
//

class thread_ct {
public:
  TSD_st* tsd;                  	// Pointer to thread specific data of this thread or NULL when thread is no longer running.
  mutex_ct thread_mutex;		// Mutex for the attributes of this object.
  void* memblk_map;             	// Pointer to memblk_map_ct of this thread.
  dm_alloc_ct* base_alloc_list;		// The base list with `dm_alloc_ct' objects.  Each of these objects has a list of it's own.
  dm_alloc_ct** current_alloc_list;	// The current list to which newly allocated memory blocks are added.
  dm_alloc_ct* current_owner_node;	// If the current_alloc_list != &base_alloc_list, then this variable
					// points to the dm_alloc_ct node who owns the current list.
  size_t memsize;			// Total number of allocated bytes (excluding internal allocations).
  unsigned long memblks;		// Total number of allocated blocks (excluding internal allocations).
  pthread_t tid;			// Thread ID.

  thread_ct(TSD_st* tsd_ptr) throw();
  void initialize(TSD_st* tsd_ptr) throw();
  void tsd_destroyed(void) throw();
  bool is_zombie(void) const { return !tsd; }
};

// The list of threads.
// New thread objects are added in TSD_st::S_initialize.
typedef std::list<thread_ct, internal_allocator::rebind<thread_ct>::other> threadlist_t;
extern threadlist_t* threadlist;

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCW_PRIVATE_THREAD_H

