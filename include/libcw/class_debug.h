// $Header$
//
// Copyright (C) 2000 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file class_debug.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_DEBUG_H
#define LIBCW_CLASS_DEBUG_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_SET_H
#include <libcw/class_channel_set.h>
#endif
#ifndef LIBCW_PRIVATE_STRUCT_TSD_H
#include <libcw/private_struct_TSD.h>
#endif
#ifndef LIBCW_STRUCT_DEBUG_TSD
#include <libcw/struct_debug_tsd.h>
#endif
#ifndef LIBCW_PRIVATE_LOCK_INTERFACE_H
#include <libcw/private_lock_interface.h>
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif

namespace libcw {
  namespace debug {

#if CWDEBUG_ALLOC
namespace _private_ {

struct debug_message_st {
  struct debug_message_st* next;
  struct debug_message_st* prev;
  int curlen;
  char buf[sizeof(int)];
};

}
#endif

//===================================================================================================
// class debug_ct
//
// Note: Debug output is printed already *before* this object is constructed,
// and is still printed when this object is already destructed.
// This is why initialization is done with method init() *before* construction
// and debug is turned off when this object is destructed.
// I hope that this is no problem because debug::libcw_do is a global object.
// It means however that this object can not contain any attributes that have
// a constructor of their own!

/**
 * \class debug_ct debug.h libcw/debug.h
 * \ingroup group_debug_object
 *
 * \brief The %Debug Object class, this object represents one output device (<code>ostream</code>).
 *
 * See \ref group_debug_object.
 */
class debug_ct {
  friend void debug_tsd_st::start(debug_ct&, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM);
  friend void debug_tsd_st::finish(debug_ct &, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM);
#ifdef LIBCW_DOXYGEN
protected:
#else
public: // Only public because macro LibcwDout needs acces, don't access this directly.
#endif
#ifndef _REENTRANT
  //-------------------------------------------------------------------------------------------------
  // Put the otherwise Thread Specific Data of this debug object
  // directly into the object when we don't use threads.
  //

  debug_tsd_st tsd;
#else
  int WNS_index;
  static int S_index_count;
#endif

protected:
  //-------------------------------------------------------------------------------------------------
  // Protected attributes.
  //

  std::ostream* real_os;
    // The original output ostream (as set with set_ostream()).
    //
#ifdef _REENTRANT
  _private_::lock_interface_base_ct* M_mutex;
    // Pointer to the mutex that should be used for `real_os' or NULL when no lock is used.
    // A value of NULL is only allowed prior to creating a second thread.
#endif

private:
  //-------------------------------------------------------------------------------------------------
  // Private attributes: 
  //

  bool WNS_initialized;
    // Set to true when this object is initialized (by a call to NS_init()).

#if CWDEBUG_DEBUG
  long init_magic;
    // Used to check if the trick with `WNS_initialized' really works.
#endif

  bool interactive;
    // Set true if the last or current debug output is to cerr

#if CWDEBUG_ALLOC
public:
  _private_::debug_message_st* queue;
  _private_::debug_message_st* queue_top;
    // Queue of messages written inside malloc/realloc/calloc/free/new/delete.
    // Locked by mutex provided through set_ostream.
#endif

public:
  /** \addtogroup group_formatting */
  /** \{ */

  /**
   * \brief The margin
   *
   * This is printed before the label.&nbsp;
   * The margin can be manipulated directly using the methods of class debug_string_ct.
   *
   * \sa push_margin()
   *  \n pop_margin()
   */
  debug_string_ct& margin(void);
  debug_string_ct const& margin(void) const;

  /**
   * \brief The marker
   *
   * This is printed after the label.&nbsp;
   * The marker can be manipulated directly using the methods of class debug_string_ct.
   *
   * \sa push_marker()
   *  \n pop_marker()
   */
  debug_string_ct& marker(void);
  debug_string_ct const& marker(void) const;

  /** \} */

public:
  //-------------------------------------------------------------------------------------------------
  // Manipulators and accessors for the "format" attributes:
  //

  void set_indent(unsigned short indentation);
  void inc_indent(unsigned short indentation);
  void dec_indent(unsigned short indentation);
  unsigned short get_indent(void) const;

  void push_margin(void);
  void pop_margin(void);
  void push_marker(void);
  void pop_marker(void);

  // Deprecated (last documented in 0.99.15)
  void set_margin(std::string const& s);
  void set_marker(std::string const& s);
  std::string get_margin(void) const;
  std::string get_marker(void) const;

  //-------------------------------------------------------------------------------------------------
  // Other accessors
  //

  std::ostream* get_ostream(void) const;		// The orignial ostream set with set_ostream.

private:
  //-------------------------------------------------------------------------------------------------
  // Initialization function.
  //

  friend class channel_ct;
  friend class fatal_channel_ct;
  friend void ST_initialize_globals(void);
#if CWDEBUG_LOCATION
  friend bool cwbfd::ST_init(void);
#endif
  void NS_init(void);
    // Initialize this object, needed because debug output can be written
    // from the constructors of (other) global objects, and from the malloc()
    // family when CWDEBUG_ALLOC is set to 1.

public:
  //-------------------------------------------------------------------------------------------------
  // Constructors and destructors.
  //

  debug_ct(void);

private:
  void private_set_ostream(std::ostream* os);

public:
  //-------------------------------------------------------------------------------------------------
  // Manipulators:
  //

  void set_ostream(std::ostream* os);
#if defined(_REENTRANT) || defined(LIBCW_DOXYGEN)
  template<class T>
    void set_ostream(std::ostream* os, T* mutex);
#ifdef LIBCW_DOXYGEN
  // Specialization.
  template<>
    void set_ostream(std::ostream* os, pthread_mutex_t* mutex);
#endif
#endif
  void off(void);
  void on(void);
  
  struct OnOffState {
    int _off;
#if CWDEBUG_DEBUGOUTPUT
    bool first_time;
#endif
  };

  void force_on(OnOffState& state);
  void restore(OnOffState const& state);

#if defined(_REENTRANT) || defined(LIBCW_DOXYGEN)
  /**
   * \brief Keep Thread Specific Data after thread exit.
   *
   * Each %debug object has an array (of size PTHREAD_THREADS_MAX (= 1024))
   * with pointers to the Thread Specific Data (TSD) for each thread id.&nbsp;
   * The size of this TSD is about 640 bytes.
   *
   * When this data would not be freed when a thread exits, and when
   * an application constantly creates new threads, then all 1024 entries
   * will become used, causing in total 1024 * 640 bytes of memory to be used
   * per %debug object instead of <i>number of running threads</i> * 640 bytes.
   *
   * However, freeing the Thread Specific Data can not be done at the
   * very very last moment (for which there is a complicated reason).&nbsp;
   * Therefore, libcwd deletes the TSD of %debug objects in <code>__pthread_destroy_specifics()</code>,
   * which is called directly after <code>__pthread_perform_cleanup()</code> in <code>pthread_exit()</code>.&nbsp;
   * <code>__pthread_destroy_specifics()</code> calls the destruction routines as set
   * by <code>pthread_key_create()</code>.&nbsp;
   * As you should know, the order in which these destruction routines
   * are called is not specified.&nbsp;
   * Therefore it is possible that additional %debug output done
   * in other key destruction routines is lost.
   *
   * By calling <code>keep_tsd(true)</code>, the TSD is not deleted and %debug output
   * will stay enabled till the very end.&nbsp;
   * Because of the disadvantage that this costs about 640 kb of (swappable) memory,
   * the default is <code>false</code> and the TSD will be freed, except for the
   * default %debug object \ref libcw_do, allowing for the printing of calls to
   * <code>%free()</code> done after the key destruction phase.
   *
   * \returns The previous value.
   */
  bool keep_tsd(bool keep);
#endif // _REENTRANT
};

#if defined(_REENTRANT) && !defined(LIBCW_DOXYGEN)
// Specialization.
template<>
  void debug_ct::set_ostream(std::ostream* os, pthread_mutex_t* mutex);
#endif

  }  // namespace debug
}  // namespace libcw

#ifndef LIBCW_SET_OSTREAM_INL
#include <libcw/set_ostream.inl>
#endif

#endif // LIBCW_CLASS_DEBUG_H

