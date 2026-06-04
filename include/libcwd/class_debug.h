// SPDX-FileCopyrightText: 2000-2005, 2018-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file class_debug.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_DEBUG_H
#define LIBCWD_CLASS_DEBUG_H

#include "libcwd/config.h"
#include "class_channel_set.h"
#include "private_struct_TSD.h"
#include "struct_debug_tsd.h"
#include "private_lock_interface.h"
#include "threadsafe/threadsafe.h"
#include <iosfwd>
#include <atomic>
#include <mutex>

namespace libcwd {

class buffer_ct;

namespace _private_ {

// Store the ostream destination together with the lock that protects writes to it.
//
// real_os is the destination selected with debug_ct::set_ostream. mutex points at the user-provided lock
// adapter for that stream, or is null while the debug object is still limited to single-threaded output.
struct ostream_state_ct
{
  std::ostream* real_os{};
  lock_interface_base_ct* mutex{};
};

using ostream_state_ts =
    threadsafe::Unlocked<ostream_state_ct, threadsafe::policy::Primitive<std::mutex>>;

} // namespace _private_

//===================================================================================================
// class debug_ct
//
// Note: Debug output is printed already *before* this object is constructed,
// and is still printed when this object is already destructed.
// This is why initialization is done with method init() *before* construction
// and debug is turned off when this object is destructed.
// I hope that this is no problem because libcwd::libcw_do is a global object.
// It means however that this object can not contain any attributes that have
// a constructor of their own!

/**
 * \class debug_ct debug.h libcwd/debug.h
 * \ingroup group_debug_object
 *
 * \brief The %Debug Object class, this object represents one output device (<code>ostream</code>).
 *
 * See \ref group_debug_object.
 */
class debug_ct {
  friend void debug_tsd_st::start(debug_ct&, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM);
  friend void debug_tsd_st::finish(debug_ct &, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM);
#ifdef LIBCWD_DOXYGEN
protected:
#else
public: // Only public because macro LibcwDout needs acces, don't access this directly.
#endif
  int WNS_index;
  static int S_index_count;

protected:
  //-------------------------------------------------------------------------------------------------
  // Protected attributes.
  //

  friend class libcwd::buffer_ct;		// buffer_ct::writeto() needs access.
  _private_::ostream_state_ts ostream_state_;
    // Lazily allocated ostream destination state and matching external lock, protected as one unit.
    // This remains a pointer because debug_ct storage may be used by initialize() before the constructor runs.

  buffer_ct* unfinished_oss;
  void const* newlineless_tsd;

private:
  //-------------------------------------------------------------------------------------------------
  // Private attributes:
  //

  bool WNS_initialized;
    // Set to true when this object is initialized (by a call to NS_init()).

  bool NS_being_initialized;
    // Set to true when this object is being initialized (by a call to NS_init()).

#if CWDEBUG_DEBUG
  long init_magic;
    // Used to check if the trick with `WNS_initialized' really works.
#endif

  bool interactive;
    // Set true if the last or current debug output is to cerr

  std::atomic<int> m_always_flush{0};
    // Application-wide flush on/off counter for Debug Objects.


public:
  /** \addtogroup group_formatting */
  /** \{ */

  /**
   * \brief Colorization code
   *
   * This is printed before the margin.&nbsp;
   * The color_on string can be manipulated directly using the methods of class debug_string_ct.
   */
  debug_string_ct& color_on();
  debug_string_ct const& color_on() const;

  /**
   * \brief Turn colorization off
   *
   * This is printed before the newline.&nbsp;
   * The color_off string can be manipulated directly using the methods of class debug_string_ct.
   */
  debug_string_ct& color_off();
  debug_string_ct const& color_off() const;

  /**
   * \brief The margin
   *
   * This is printed before the label.&nbsp;
   * The margin can be manipulated directly using the methods of class debug_string_ct.
   *
   * \sa push_margin()
   *  \n pop_margin()
   */
  debug_string_ct& margin();
  debug_string_ct const& margin() const;

  /**
   * \brief The marker
   *
   * This is printed after the label.&nbsp;
   * The marker can be manipulated directly using the methods of class debug_string_ct.
   *
   * \sa push_marker()
   *  \n pop_marker()
   */
  debug_string_ct& marker();
  debug_string_ct const& marker() const;

  /** \} */

public:
  //-------------------------------------------------------------------------------------------------
  // Manipulators and accessors for the "format" attributes:
  //

  void set_indent(unsigned short indentation);
  void inc_indent(unsigned short indentation);
  void dec_indent(unsigned short indentation);
  unsigned short get_indent() const;

  void push_margin();
  void pop_margin();
  void push_marker();
  void pop_marker();

  //-------------------------------------------------------------------------------------------------
  // Other accessors
  //

  std::ostream* get_ostream() const;		// The original ostream set with set_ostream.
  bool has_mutex() const;                       // Returns true if a mutex was (already) set with set_ostream.

private:
  //-------------------------------------------------------------------------------------------------
  // Initialization function.
  //

  friend class channel_ct;
  friend class fatal_channel_ct;
  friend void ST_initialize_globals(LIBCWD_TSD_PARAM);
#if CWDEBUG_LOCATION
  friend bool dwarf::ensure_initialization(LIBCWD_TSD_PARAM);
#endif
  bool NS_init(LIBCWD_TSD_PARAM);
    // Initialize this object, needed because debug output can be written
    // from the constructors of (other) global objects.
    // This will return false when it is called recursively which can happen
    // as part of initialization of libcwd via a call to malloc while creating
    // laf_ct -> buffer_ct --> basic_stringbuf.  In that case the initialization
    // failed thus.  On success, it returns true.

public:
  //-------------------------------------------------------------------------------------------------
  // Constructors and destructors.
  //

  debug_ct();

private:
  // Store os in the already locked ostream_state for this debug object.
  //
  // The caller keeps the write access object alive while calling this helper so the ostream pointer remains
  // synchronized with the stream lock pointer that readers use for the lifetime handoff.
  void private_set_ostream(_private_::ostream_state_ct& ostream_state, std::ostream* os);

public:
  //-------------------------------------------------------------------------------------------------
  // Manipulators:
  //

  void set_ostream(std::ostream* os);
  template<class T>
    void set_ostream(std::ostream* os, T* mutex);
#ifdef LIBCWD_DOXYGEN
  // Specialization.
  template<>
    void set_ostream(std::ostream* os, pthread_mutex_t* mutex);
#endif
  void off();
  void on();

  void always_flush_on();
  void always_flush_off();

  struct OnOffState {
    int _off;
#if CWDEBUG_DEBUGOUTPUT
    bool first_time;
#endif
  };

  void force_on(OnOffState& state);
  void restore(OnOffState const& state);
  bool is_on(LIBCWD_TSD_PARAM) const;
  bool always_flush_is_on() const;
};

#if !defined(LIBCWD_DOXYGEN)
// Specialization.
template<>
  void debug_ct::set_ostream(std::ostream* os, pthread_mutex_t* mutex);
#endif

}  // namespace libcwd

#include "set_ostream.inl"

#endif // LIBCWD_CLASS_DEBUG_H
