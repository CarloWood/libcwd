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

/** \file class_location.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_LOCATION_H
#define LIBCW_CLASS_LOCATION_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif

#if CWDEBUG_LOCATION

#ifndef LIBCW_PRIVATE_STRUCT_TSD_H
#include <libcw/private_struct_TSD.h>
#endif
#ifndef LIBCW_CLASS_OBJECT_FILE_H
#include <libcw/class_object_file.h>
#endif
#ifndef LIBCW_LOCKABLE_AUTO_PTR_H
#include <libcw/lockable_auto_ptr.h>
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
 *     location_ct((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset) );
 * \endcode
 */
class location_ct {
protected:
  lockable_auto_ptr<char, true> M_filepath;	//!< The full source file name of this location.&nbsp; Allocated in `M_pc_location' using new [].
  union {
    char* M_filename;				//!< Points inside M_filepath just after the last '/' or to the beginning.
    void const* M_initialization_delayed;	//!< If M_object_file == NULL and M_func points to S_pre_ios_initialization_c or S_pre_libcwd_initialization_c, then this is the address that M_pc_location was called with.
  };
  unsigned int M_line;				//!< The line number of this location.
  char const* M_func;				//!< Pointer to static string containing the mangled function name of this location.
  object_file_ct const* M_object_file;		//!< A pointer to an object representing the library or executable that this location belongs to or NULL when not initialized.
  bool M_known;					//!< Set when M_filepath (and M_filename) point to valid data and M_line contains a valid line number.
private:
#if CWDEBUG_ALLOC
  friend class ooam_filter_ct;
  bool M_hide;					// Used by ooam_filter_ct::M_sychronize_locations
#endif

protected:
  // M_func can point to one of these constants, or to libcw::debug::unknown_function_c
  // or to a static string with the mangled function name.
  static char const* const S_uninitialized_location_ct_c;
  static char const* const S_pre_ios_initialization_c;
  static char const* const S_pre_libcwd_initialization_c;
  static char const* const S_cleared_location_ct_c;

public:
  location_ct(void const* addr);
      // Construct a location object for address `addr'.
#if LIBCWD_THREAD_SAFE
  location_ct(void const* addr LIBCWD_COMMA_TSD_PARAM);
      // Idem, but with passing the TSD.
#endif
  ~location_ct();

  /**
   * \brief The default constructor.
   *
   * Constructs an unknown location object.
   * Use \ref pc_location to initialize the object.
   */
  location_ct(void);

  /**
   * \brief Copy constructor.
   *
   * Constructs a location that is equivalent to the location passed as argument.
   * The new object will be the owner of the memory allocations of this location
   * unless the member function \ref lock_ownership was called for the prototype
   * before copying it.  It is the responsibility of the coder to make sure that
   * the allocation owner is the object that is last deleted (if at all).
   */
  location_ct(location_ct const& location);

  /**
   * \brief Assignment operator.
   *
   * Assigns the value of the location passed to the current object.
   * The new object will be the owner of the memory allocations of this location
   * unless the member function \ref lock_ownership was called for the prototype
   * before copying it.  It is the responsibility of the coder to make sure that
   * the allocation owner is the object that is last deleted (if at all).
   */
  location_ct& operator=(location_ct const& location);		// Assignment operator

  /**
   * \brief Lock ownership of internal allocations.
   *
   * Makes this object responsible for deleting internal allocations,
   * the user is responsible to making sure that the owner is
   * deleted last.
   */
  void lock_ownership(void) { if (M_known) M_filepath.lock(); }

  /**
   * \brief Initialize the current object with the location that corresponds with \a pc.
   */
  void pc_location(void const* pc);

  // Only public because libcwd calls it directly.
  void M_pc_location(void const* addr LIBCWD_COMMA_TSD_PARAM);

  /**
   * \brief Clear the current object (set the location to 'unknown').
   */
  void clear(void);

public:
  // Accessors
  /**
    * \brief Returns <CODE>false</CODE> if no source-%file:line-number information is known for this location
    * (or when it is uninitialized or clear()-ed).
    */
  bool is_known(void) const;

  /**
   * \brief The source file name (without path).
   *
   * We don't allow to retrieve a pointer to the allocated character string because
   * that is dangerous as the memory that it is pointing to could be deleted.
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

  /** \brief The size of the file name. */
  size_t filename_length(void) const { return M_known ? strlen(M_filename) : 0; }
  /** \brief The size of the full path name. */
  size_t filepath_length(void) const { return M_known ? strlen(M_filepath.get()) : 0; }

  /** \brief Corresponding object file.
   *
   * Returns a pointer to an object representing the shared library or the executable
   * that this location belongs to; only valid if is_known() returns true.
   */
  object_file_ct const* object_file(void) const { return M_object_file; }

  // Printing
  /** \brief Write the full path to an ostream. */
  void print_filepath_on(std::ostream& os) const;
  /** \brief Write the file name to an ostream. */
  void print_filename_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, location_ct const& location);
      // Prints a default "M_filename:M_line".

  // This is used in list_allocations_on.
  bool initialization_delayed(void) const { return (!M_object_file && (M_func == S_pre_ios_initialization_c || M_func == S_pre_libcwd_initialization_c)); }
#if CWDEBUG_ALLOC
  void handle_delayed_initialization(ooam_filter_ct const& filter);
  bool hide_from_alloc_list(void) const { return M_hide; }
#endif
};

/** \} */ // End of group 'group_locations'

  } // namespace debug
} // namespace libcw

#endif // CWDEBUG_LOCATION
#endif // LIBCW_CLASS_LOCATION_H
