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
#ifndef LIBCW_PRIVATE_TSD_H
#include <libcw/private_TSD.h>
#endif
#ifndef LIBCW_STRUCT_DEBUG_TSD
#include <libcw/struct_debug_tsd.h>
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif

namespace libcw {
  namespace debug {

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
#ifndef LIBCWD_THREAD_SAFE
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
    // The original output ostream (as set with set_ostream() or set_fd()).

private:
  //-------------------------------------------------------------------------------------------------
  // Private attributes: 
  //

  bool WNS_initialized;
    // Set to true when this object is initialized (by a call to NS_init()).

#ifdef DEBUGDEBUG
  long init_magic;
    // Used to check if the trick with `WNS_initialized' really works.
#endif

  bool interactive;
    // Set true if the last or current debug output is to cerr

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
#ifdef DEBUGUSEBFD
  friend bool cwbfd::ST_init(void);
#endif
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
  
  struct OnOffState {
    int _off;
#ifdef DEBUGDEBUGOUTPUT
    bool first_time;
#endif
  };

  void force_on(OnOffState& state);
  void restore(OnOffState const& state);
};

  }  // namespace debug
}  // namespace libcw

#endif // LIBCW_CLASS_DEBUG_H

