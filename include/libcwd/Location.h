// SPDX-FileCopyrightText: 2000-2005, 2018-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_LOCATION_H
#define LIBCWD_CLASS_LOCATION_H

#include "libcwd/config.h"

#if CWDEBUG_LOCATION

#include "ObjectFileName.h"
#include "lockable_auto_ptr.h"
#include "private/ThreadSpecificData.h"

#include <cstring>
#include <iosfwd>
#include <string>

namespace libcwd {

// Forward declaration.
class Location;

namespace _private_ {

// Forward declaration.
template <class OSTREAM>
void print_location_on(OSTREAM& os, Location const& location);

} // namespace _private_
} // namespace libcwd

namespace libcwd {

/** @addtogroup group_locations */
/** \{ */

/** @brief This constant (pointer) is returned by Location::mangled_function_name() when no function is known. */
extern char const* const unknown_function_c;

/**
 * @class Location Location.h libcwd/debug.h
 * @brief A source file location.
 *
 * The normal usage of this class is to print *source-name*:*line-number* information as follows:
 * ```cpp
 * Dout(dc::notice, "Called from " <<
 *     Location((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset) );
 * ```
 */
class Location
{
 protected:
  ObjectFileName const* object_file_name_; //!< A pointer to an object representing the library or executable that this
                                           //!< location belongs to or NULL when not initialized.
  char const* function_name_; //!< Pointer to static string containing the mangled function name of this location.
  lockable_auto_ptr<char, true>
      filepath_; //!< The full source file name of this location. Allocated in `pc_location' using new [].
  union
  {
    char const* filename_; //!< Points inside filepath_ just after the last '/' or to the beginning.
    void const* initialization_delayed_; //!< If object_file_name_ == NULL and function_name_ points to
                                         //!< pre_libcwd_initialization_c_, then this is the address that pc_location
                                         //!< was called with.
    void const* unknown_pc_; //!< If object_file_name_ == NULL and function_name_ points to unknown_function_c, then
                             //!< this is the address that pc_location was called with.
  };
  unsigned int line_; //!< The line number of this location.
  bool known_; //!< Set when filepath_ (and filename_) point to valid data and line_ contains a valid line number.
 private:
 protected:
  // function_name_ can point to one of these constants, or to libcwd::unknown_function_c
  // or to a static string with the mangled function name.
  static char const uninitialized_location[];
  static char const pre_libcwd_initialization[];
  static char const cleared_location[];

 public:
  explicit Location(void const* addr);
  // Construct a location object for address `addr'.
  explicit Location(void const* addr, LIBCWD_TSD_PARAM);
  // Idem, but with passing the TSD.
  ~Location();

  /**
   * @brief The default constructor.
   *
   * Constructs an unknown location object.
   * Use @ref pc_location to initialize the object.
   */
  Location();

  /**
   * @brief Copy constructor.
   *
   * Constructs a location that is equivalent to the location passed as argument.
   * Copies share the stored source-file path. By default, ownership of that path
   * moves to the copy; call @ref lock_ownership on the prototype first when the
   * prototype must remain responsible for releasing the path storage.
   */
  Location(Location const& location);

  /**
   * @brief Assignment operator.
   *
   * Assigns the value of the location passed to the current object. Copies share
   * the stored source-file path. By default, ownership of that path moves to the
   * target object; call @ref lock_ownership on the source first when the source
   * must remain responsible for releasing the path storage.
   */
  Location& operator=(Location const& location); // Assignment operator

  /**
   * @brief Keep ownership of the stored path in this object.
   *
   * Prevents a subsequent copy or assignment from taking over responsibility for
   * releasing the source-file path owned by this location. Use this when a
   * location is used as a prototype for shorter-lived copies.
   */
  void lock_ownership()
  {
    if (known_)
      filepath_.lock();
  }

  /**
   * @brief Initialize the current object with the location that corresponds with @p pc.
   */
  void pc_location(void const* pc);

  // Only public because libcwd calls it directly.
  void pc_location(void const* pc, LIBCWD_TSD_PARAM);

  /**
   * @brief Clear the current object (set the location to 'unknown').
   */
  void clear();

 public:
  // Accessors
  /**
   * @brief Returns `false` if no *source-file*:*line-number* information is known for this location
   * (or when it is uninitialized or clear()-ed).
   */
  bool is_known() const;

  /**
   * @brief The source file name (without path).
   *
   * We don't allow to retrieve a pointer to the allocated character string because
   * that is dangerous as the memory that it is pointing to could be deleted.
   */
  std::string file() const;

  /** @brief Return the line number; only valid if is_known() returns true. */
  unsigned int line() const;

  /** @brief Returns the mangled function name or @ref unknown_function_c when no function could be found.
   *
   * Two other strings that can be returned are "<uninitialized Location>" and
   * "<cleared Location>", the idea is to never print that: you should know it when a
   * location object is in these states.
   */
  char const* mangled_function_name() const;

  /** @brief The size of the file name. */
  size_t filename_length() const { return known_ ? std::strlen(filename_) : 0; }
  /** @brief The size of the full path name. */
  size_t filepath_length() const { return known_ ? std::strlen(filepath_.get()) : 0; }

  /** @brief Corresponding object file.
   *
   * Returns a pointer to an object representing the shared library or the executable
   * that this location belongs to; only valid if is_known() returns true.
   */
  ObjectFileName const* object_file() const { return object_file_name_; }

  // Printing
  /** @brief Write the full path to an ostream. */
  void print_filepath_on(std::ostream& os) const;
  /** @brief Write the file name to an ostream. */
  void print_filename_on(std::ostream& os) const;
  template <class OSTREAM>
  friend void _private_::print_location_on(OSTREAM& os, Location const& location);
#if (__GNUC__ == 3 && __GNUC_MINOR__ < 4)
  // This doesn't need to be a friend, but g++ 3.3.x and lower are broken.
  // We need to declare an operator<< this way as a workaround.
  friend std::ostream& operator<<(std::ostream& os, Location const& location)
  {
    _private_::print_location_on(os, location);
    return os;
  }
#endif

  // Return the program counter that still needs lazy symbol resolution, if any.
  bool initialization_delayed() const { return (!object_file_name_ && function_name_ == pre_libcwd_initialization); }
  void const* unknown_pc() const
  {
    return (!object_file_name_ && function_name_ == unknown_function_c) ? unknown_pc_
           : initialization_delayed()                                   ? initialization_delayed_
                                                                        : 0;
  }
};

// #if (__GNUC__ > 3 || __GNUC_MINOR__ >= 4)
// extern std::ostream& operator<<(std::ostream& os, Location const& location);
// #endif

/** @brief Set the output format of Location.
 *
 * This function can be used to specify the format of how a Location will be printed
 * when it is written to an ostream.  The format is thread-specific: only the calling
 * thread will be influenced.
 *
 * The argument @p format is a bit-wise OR-ed value of three possible bit masks:
 * `show_function` : Include the mangled function name.
 * `show_object` : Include the name of the shared library or the executable name.
 * `show_path` : Print the full path of the source file.
 *
 * @returns the previous value of the format.
 */
location_format_t location_format(location_format_t format);

/** \} */ // End of group 'group_locations'

} // namespace libcwd

#endif // CWDEBUG_LOCATION
#endif // LIBCWD_CLASS_LOCATION_H
