// $Header$
//
// Copyright (C) 2000 - 2004, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_CLASS_LOCATION_INL
#define LIBCWD_CLASS_LOCATION_INL

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif

#if CWDEBUG_LOCATION

#ifndef LIBCWD_LIBRARIES_DEBUG_H
#include <libcwd/libraries_debug.h>
#endif
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif
#ifndef LIBCWD_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif
#ifndef LIBCWD_CLASS_LOCATION_H
#include <libcwd/class_location.h>
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif

namespace libcwd {

/**
 * \brief Construct a location for address \p addr.
 */
inline
location_ct::location_ct(void const* addr) : M_known(false)
#if CWDEBUG_ALLOC
    , M_hide(_private_::new_location)
#endif
{
  LIBCWD_TSD_DECLARATION;
  M_pc_location(addr LIBCWD_COMMA_TSD);
}

#if LIBCWD_THREAD_SAFE
/*
 * Construct a location for address addr,
 * taking a thread-specific-data argument.
 */
inline
location_ct::location_ct(void const* addr LIBCWD_COMMA_TSD_PARAM) : M_known(false)
#if CWDEBUG_ALLOC
     , M_hide(_private_::new_location)
#endif
{
  M_pc_location(addr LIBCWD_COMMA_TSD);
}
#endif

/**
 * \brief Destructor.
 */
inline
location_ct::~location_ct()
{
  clear();
}

inline
location_ct::location_ct(void) : M_func(S_uninitialized_location_ct_c), M_object_file(NULL), M_known(false)
#if CWDEBUG_ALLOC
    , M_hide(_private_::new_location)
#endif
{
}

/**
 * \brief Point this location to a different program counter address.
 */
inline
void
location_ct::pc_location(void const* addr)
{
  clear();
  LIBCWD_TSD_DECLARATION;
  M_pc_location(addr LIBCWD_COMMA_TSD);
}

inline
bool
location_ct::is_known(void) const
{
  return M_known;
}

inline
std::string
location_ct::file(void) const
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( !__libcwd_tsd.internal );		// Returning a string, using a userspace allocator!
#endif
  LIBCWD_ASSERT( M_known );
  return M_filename;
}

inline
unsigned int
location_ct::line(void) const
{
  LIBCWD_ASSERT( M_known );
  return M_line;
}

inline
char const*
location_ct::mangled_function_name(void) const
{
  return M_func;
}

inline
location_format_t
location_format(location_format_t format)
{
  LIBCWD_TSD_DECLARATION;
  location_format_t ret = __libcwd_tsd.format;
  __libcwd_tsd.format = format;
  return ret;
}

namespace _private_ {

template<class OSTREAM>
  void print_location_on(OSTREAM& os, location_ct const& location)
  {
    if (location.M_known)
    {
      LIBCWD_TSD_DECLARATION;
      if ((__libcwd_tsd.format & show_objectfile))
	os << location.M_object_file->filename() << ':';
      if ((__libcwd_tsd.format & show_function))
	os << location.M_func << ':';
      if ((__libcwd_tsd.format & show_path))
	os << location.M_filepath.get() << ':' << location.M_line;
      else
	os << location.M_filename << ':' << location.M_line;
    }
    else
      os << location.M_object_file->filename() << ':' << location.M_func;
  }

} // namespace _private_

#if !(__GNUC__ == 3 && __GNUC_MINOR__ < 4)	// See class_location.h
/**
 * \brief Write \a location to ostream \a os.
 * \ingroup group_locations
 *
 * Write the contents of a location_ct object to an ostream in the form <i>source-file</i>:<i>line-number</i>,
 * or writes <i>objectfile</i>:<i>mangledfuncname</i> when the location is unknown.
 * If the <i>source-file</i>:<i>line-number</i> is known, then it may be prepended by the object file
 * and/or the mangled function name anyway if this was requested through \ref location_format.
 * That function can also be used to cause the <i>source-file</i> to be printed with its full path.
 */
inline
std::ostream&
operator<<(std::ostream& os, location_ct const& location)
{
  _private_::print_location_on(os, location);
  return os;
}
#endif

} // namespace libcwd

#endif // CWDEBUG_LOCATION
#endif // LIBCWD_CLASS_LOCATION_INL
