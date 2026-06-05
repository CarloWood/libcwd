// SPDX-FileCopyrightText: 2002-2005, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_thread.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_THREAD_H
#define LIBCWD_PRIVATE_THREAD_H

#include <list>

namespace libcwd {

class dm_alloc_ct;

namespace _private_ {

struct TSD_st;
struct ThreadsWat;

// class thread_ct
//
// Each created thread gets an object of type `thread_ct' assigned.
// The objects are stored in an STL list so that libcwd can keep stable
// iterators while tracking thread lifetime and cancelling other threads from
// DoutFatal.
//
class thread_ct
{
 public:
  pthread_t tid;			// Thread ID, used to terminate all threads in a DoutFatal(dc::fatal, ...).
  bool M_zombie;
  bool M_terminating;

  void initialize(LIBCWD_TSD_PARAM);	// May only be called after the object reached its final place in memory.
  void terminated(ThreadsWat wat LIBCWD_COMMA_TSD_PARAM);
  bool is_zombie() const { return M_zombie; }
  void terminating() { M_terminating = true; }
  bool is_terminating() const { return M_terminating; }
};

// Own the process-wide thread list behind a private threadsafe implementation.
//
// add_current_thread stores the iterator for the newly inserted thread in __libcwd_tsd and initializes the
// thread object in its final list location. mark_thread_terminated and cancel_all_other_threads serialize their
// list traversal with insertions; callers are still responsible for disabling or deferring pthread cancellation
// when they need cancellation-safe critical sections around these operations.
struct ThreadList
{
  class impl_ct;
  impl_ct* impl_;       // Deliberately leaked.

  // The list of threads.
  // New thread objects are added while initializing the TSD for a newly observed thread.
  static ThreadList const& instance();

  // The type of the underlying container storing thread_ct objects.
  using list_type = std::list<thread_ct>;

  void add_current_thread(LIBCWD_TSD_PARAM) const;
  void mark_thread_terminated(list_type::iterator thread_iter LIBCWD_COMMA_TSD_PARAM) const;
  void cancel_all_other_threads(pthread_t current_tid) const;
};

} // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_THREAD_H
