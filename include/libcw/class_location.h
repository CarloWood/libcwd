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

/** \file class_location.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_LOCATION_H
#define LIBCW_CLASS_LOCATION_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_PRIVATE_STRUCT_TSD_H
#include <libcw/private_struct_TSD.h>
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif

namespace libcw {
  namespace debug {

/** \addtogroup group_locations */
/** \{ */

/** \brief This constant (pointer) is returned by location_ct::mangled_function_name() when no function is known. */
extern char const* const unknown_function_c;

/**
 * \class location_ct class_location.h libcw/debug.h
 * \brief A source file location.
 *
 * The normal usage of this class is to print file-name:line-number information as follows:
 * \code
 * Dout(dc::notice, "Called from " <<
 *     location_ct((char*)__builtin_return_address(0) + libcw::builtin_return_address_offset) );
 * \endcode
 */
class location_ct {
protected:
  char* M_filepath;	//!< The full source file name of this location, or NULL when unknown.&nbsp; Allocated in `M_pc_location' using new [].
  char* M_filename;	//!< Points inside M_filepath just after the last '/' or to the beginning.
  unsigned int M_line;	//!< The line number of this location.
  char const* M_func;	//!< Pointer to static string containing the mangled function name of this location.

  void M_pc_location(void const* addr LIBCWD_COMMA_TSD_PARAM);

public:
  location_ct(void const* addr);
      // Construct a location object for address `addr'.
#ifdef LIBCWD_THREAD_SAFE
  location_ct(void const* addr LIBCWD_COMMA_TSD_PARAM);
      // Idem, but with passing the TSD.
#endif
  ~location_ct();

  // Provided, but deprecated (I honestly think you never need them):
  location_ct(void);						// Default constructor
  location_ct(location_ct const& location);			// Copy constructor
  location_ct& operator=(location_ct const& location);		// Assignment operator

  void pc_location(void const* pc);
  void clear(void);
  void move(location_ct& prototype);

public:
  // Accessors
  /**
    * \brief Returns <CODE>false</CODE> if no source-%file:line-number information is known for this location
    * (or when it is uninitialized or clear()-ed).
    */
  bool is_known(void) const;

  /**
   * \brief Return the source file name (without path).  We don't allow to retrieve a pointer
   * to M_filepath; that is dangerous as the memory that it is pointing to could be deleted.
   */
  std::string file(void) const;

  /** \brief Return the line number; only valid if is_known() returns true. */
  unsigned int line(void) const;

  /** \brief Returns the mangled function name or \ref unknown_function_c when no function could be found.
   *
   * Two other strings that can be returned are "<uninitialized %location_ct>" and
   * "<cleared %location_ct>", the idea is to never print that: you should know it when a
   * location object is in these states.
   */
  char const* mangled_function_name(void) const;

  // Printing
  void print_filepath_on(std::ostream& os) const;
  void print_filename_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, location_ct const& location);
      // Prints a default "M_filename:M_line".
};

/** \} */ // End of group 'group_locations'

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_LOCATION_H
