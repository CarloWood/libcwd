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

/** \file class_object_file.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_OBJECT_FILE_H
#define LIBCWD_CLASS_OBJECT_FILE_H

#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif

namespace libcwd {

// Forward declarations.
#if CWDEBUG_ALLOC
class alloc_filter_ct;
#endif
namespace cwbfd {
  class bfile_ct;
} // namespace cwbfd

/** \addtogroup group_locations */
/** \{ */

/**
 * \class object_file_ct class_object_file.h libcwd/debug.h
 * \brief An object representing the main executable or a shared library.
 *
 * This class contains the full path (file name) of an object file.
 * As a member of class bfile_ct, defined in namespace cwbfd,
 * it is the only data exposed to the user, of that class.
 *
 * \internal
 * Also exposed are two mutable booleans:
 * hide_from_alloc_list() returns true when allocation done by this object file should be hidden, and
 * has_no_debug_line_sections() returns true when this object file does not have debug info.
 */
class object_file_ct {
private:
  char const* M_filepath;	// The full path to the object file (internally allocated and leaking memory).
  char const* M_filename;	// Points inside M_filepath just after the last '/' or to the beginning.
#if CWDEBUG_ALLOC
  friend class alloc_filter_ct;
  mutable bool M_hide;
#endif
  mutable bool M_no_debug_line_sections;

protected:
  friend class cwbfd::bfile_ct;
  object_file_ct(char const* filepath);

public:
  /** \brief The full path name of the loaded executable or shared library. */
  char const* filepath(void) const { return M_filepath; }
  /** \brief The file name of the loaded executable or shared library (with path stripped off). */
  char const* filename(void) const { return M_filename; }

  // For internal use.
#if CWDEBUG_ALLOC
  bool hide_from_alloc_list() const { return M_hide; }
#endif
  void set_has_no_debug_line_sections(void) const { M_no_debug_line_sections = true; }
  bool has_no_debug_line_sections(void) const { return M_no_debug_line_sections; }
};

/** \} */ // End of group 'group_locations'

} // namespace libcwd

#endif // LIBCWD_CLASS_OBJECT_FILE_H
