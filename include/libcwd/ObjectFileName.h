// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_OBJECT_FILE2_H
#define LIBCWD_CLASS_OBJECT_FILE2_H

#include <string>

namespace libcwd {

// Forward declarations.
namespace dwarf {
class ObjectFileRegistry;
} // namespace dwarf

/** \addtogroup group_locations */
/** \{ */

/**
 * \class ObjectFileName ObjectFileName.h libcwd/debug.h
 * \brief An object representing the main executable or a shared library.
 *
 * This class contains the full path (file name) of an object file.
 * As a member of class ObjectFileRegistry, defined in namespace dwarf,
 * it is the only data exposed to the user, of that class.
 *
 * \internal
 * Also exposed is a mutable flag that records whether this object file lacks
 * debug line information.
 */
class ObjectFileName
{
 private:
  char const* filepath_; // The full path to the object file (internally allocated and leaking memory).
  char const* filename_; // Points inside filepath_ just after the last '/' or to the beginning.
  mutable bool no_debug_line_sections_;

 protected:
  friend class dwarf::ObjectFileRegistry;
  ObjectFileName(char const* filepath);

 public:
  /** \brief The full path name of the loaded executable or shared library. */
  char const* filepath() const { return filepath_; }
  /** \brief The file name of the loaded executable or shared library (with path stripped off). */
  char const* filename() const { return filename_; }

  // For internal use.
  void set_has_no_debug_line_sections() const { no_debug_line_sections_ = true; }
  bool has_no_debug_line_sections() const { return no_debug_line_sections_; }
};

/** \} */ // End of group 'group_locations'

} // namespace libcwd

#endif // LIBCWD_CLASS_OBJECT_FILE2_H
