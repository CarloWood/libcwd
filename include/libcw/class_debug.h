// $Header$
//
// Copyright (C) 2000 - 2001, by
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
#ifndef LIBCW_PRIVATE_DEBUG_STACK_H
#include <libcw/private_debug_stack.h>
#endif
#ifndef LIBCW_CLASS_DEBUG_STRING_H
#include <libcw/class_debug_string.h>
#endif
#ifndef LIBCW_PRIVATE_TSD_H
#include <libcw/private_TSD.h>
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif

namespace libcw {
  namespace debug {

class channel_ct;
class fatal_channel_ct;
class continued_channel_ct;
class laf_ct;

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
#ifdef DOXYGEN
protected:
#else
public: // Direct access needed in macro LibcwDout().  Do not write to these.
#endif
  int _off;
    // Debug output is turned on when this variable is -1, otherwise it is off.

  laf_ct* current;
    // Current laf.

  std::ostream* current_oss;
    // The stringstream of the current laf.  This should *always* be equal to current->oss.
    // The reason for keeping this copy is to avoid including <sstream> in debug.h.

  union {
    channel_set_st           channel_set;
    continued_channel_set_st continued_channel_set;
  };
    // Temporary storage for the current (continued) channel set (while being assembled from
    // operator| calls).  The reason for the union is that the type of this variable is converted
    // from channel_set_st to continued_channel_set_st by a reinterpret_cast when a continued_cf,
    // dc::continued or dc::finish is detected. This allows us to make compile-time decisions (using
    // overloading).

protected:
  std::ostream* orig_os;
    // The original output ostream (as set with set_ostream() or set_fd()).

  std::ostream* os;
    // The current output ostream (may be a temporal stringstream).

  bool start_expected;
    // Set to true when start() is expected, otherwise we expect a call to finish().

  bool unfinished_expected;
    // Set to true when start() should cause a <unfinished>.

  _private_::debug_stack_tst<laf_ct*> laf_stack;
    // Store for nested debug calls.

  friend continued_channel_set_st& channel_set_st::operator|(continued_cf_nt);
    // Needs access to `initialized', `off_count' and `continued_stack'.

  int off_count;
    // Number of nested and switched off continued channels till first switched on continued channel.

  _private_::debug_stack_tst<int> continued_stack;
    // Stores the number of nested and switched off continued channels.

  debug_string_stack_element_ct* M_margin_stack;
    // Pointer to list of pushed margins.

  debug_string_stack_element_ct* M_marker_stack;
    // Pointer to list of pushed markers.

protected:
  //-------------------------------------------------------------------------------------------------
  // Attributes that determine how the prefix is printed:
  //

  unsigned short indent;
    // Position at which debug message is printed.
    // A value of 0 means directly behind the marker.

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
  debug_string_ct margin;

  /**
   * \brief The marker
   *
   * This is printed after the label.&nbsp;
   * The marker can be manipulated directly using the methods of class debug_string_ct.
   *
   * \sa push_marker()
   *  \n pop_marker()
   */
  debug_string_ct marker;

  /** \} */

public:
  //-------------------------------------------------------------------------------------------------
  // Manipulators and accessors for the above "format" attributes:
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

  std::ostream& get_os(void) const;
  std::ostream* get_ostream(void) const;

private:
  //-------------------------------------------------------------------------------------------------
  // Private attributes: 
  //

  std::ostream* saved_os;
    // The saved current output ostream while writing forced cerr.

  bool interactive;
    // Set true if the last or current debug output is to cerr

  bool WNS_initialized;
    // Set to true when this object is initialized (by a call to NS_init()).

#ifdef DEBUGDEBUG
  long init_magic;
    // Used to check if the trick with `WNS_initialized' really works.
#endif

private:
  //-------------------------------------------------------------------------------------------------
  // Initialization function.
  //

  friend class channel_ct;
  friend class fatal_channel_ct;
  friend void ST_initialize_globals(void);
  void NS_init(void);
    // Initialize this object, needed because debug output can be written
    // from the constructors of (other) global objects, and from the malloc()
    // family when DEBUGMALLOC is defined.

public:
  //-------------------------------------------------------------------------------------------------
  // Constructors and destructors.
  //

  debug_ct(void);
  ~debug_ct();

public:
  //-------------------------------------------------------------------------------------------------
  // Manipulators:
  //

  void set_ostream(std::ostream* os);
  void off(void);
  void on(void);

#ifdef DEBUGDEBUGOUTPUT
  // Since with DEBUGDEBUG defined we start with _off is -1 instead of 0,
  // we need to ignore the call to on() the first time it is called.
private:
  bool first_time;
#endif

  //=================================================================================================
public: // Only public because these are accessed from LibcwDout().
	// You should not call them directly.

  void start(LIBCWD_TSD_PARAM);
  void finish(LIBCWD_TSD_PARAM);
  void fatal_finish(LIBCWD_TSD_PARAM) __attribute__ ((__noreturn__));

  //-------------------------------------------------------------------------------------------------
  // Operators that combine channels/control bits.
  //

  channel_set_st& operator|(channel_ct const& dc);
  channel_set_st& operator&(fatal_channel_ct const& fdc);  // Using operator& just to detect
  							   // that we indeed used LibcwDoutFatal!
  continued_channel_set_st& operator|(continued_channel_ct const& cdc);

  channel_set_st& operator|(fatal_channel_ct const&);
  channel_set_st& operator&(channel_ct const&);
};

  }  // namespace debug
}  // namespace libcw

#endif // LIBCW_CLASS_DEBUG_H

