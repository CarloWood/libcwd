// $Header$
//
// Copyright (C) 2002 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcwd/private_thread.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_THREAD_H
#define LIBCWD_PRIVATE_THREAD_H

#ifndef LIBCWD_PRIVATE_MUTEX_H
#include <libcwd/private_mutex.h>
#endif
#ifndef LIBCWD_PRIVATE_ALLOCATOR_H
#include <libcwd/private_allocator.h>
#endif
#ifndef LIBCW_LIST
#define LIBCW_LIST
#include <list>
#endif

namespace libcwd {

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
#if CWDEBUG_ALLOC
typedef std::list<thread_ct, internal_allocator::rebind<thread_ct>::other> threadlist_type;
#else
typedef std::list<thread_ct> threadlist_type;
#endif

public:
  mutex_ct thread_mutex;		// Mutex for the attributes of this object.
#if CWDEBUG_ALLOC
  void* memblk_map;             	// Pointer to memblk_map_ct of this thread.
  dm_alloc_ct* base_alloc_list;		// The base list with `dm_alloc_ct' objects.  Each of these objects has a list of it's own.
  dm_alloc_ct** current_alloc_list;	// The current list to which newly allocated memory blocks are added.
  dm_alloc_ct* current_owner_node;	// If the current_alloc_list != &base_alloc_list, then this variable
					// points to the dm_alloc_ct node who owns the current list.
  size_t memsize;			// Total number of allocated bytes (excluding internal allocations).
  unsigned long memblks;		// Total number of allocated blocks (excluding internal allocations).
#endif
  pthread_t tid;			// Thread ID.  This is only used to print the ID list_allocations_on, and to
  					// terminate all threads in a DoutFatal(dc::fatal, ...).
  bool M_zombie;
  bool M_terminating;

  void initialize(LIBCWD_TSD_PARAM);	// May only be called after the object reached its final place in memory.
  void terminated(threadlist_type::iterator LIBCWD_COMMA_TSD_PARAM);
  bool is_zombie() const { return M_zombie; }
  void terminating() { M_terminating = true; }
  bool is_terminating() const { return M_terminating; }
};

// The list of threads.
// New thread objects are added in TSD_st::S_initialize.
typedef thread_ct::threadlist_type threadlist_t;
extern threadlist_t* threadlist;

  } // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_THREAD_H

