#pragma once

#ifndef LIBCWD_CLASS_LOCATION_INL
#define LIBCWD_CLASS_LOCATION_INL

#include "libcwd/config.h"

#if CWDEBUG_LOCATION

#include "class_location.h"
#include "libraries_debug.h"
#include "private_assert.h"

#include <iomanip>
#include <string>

namespace libcwd {

/**
 * \brief Construct a location for address \p addr.
 */
inline Location::Location(void const* addr) : known_(false)
{
  LIBCWD_TSD_DECLARATION;
  pc_location(addr LIBCWD_COMMA_TSD);
}

/*
 * Construct a location for address addr,
 * taking a thread-specific-data argument.
 */
inline Location::Location(void const* addr LIBCWD_COMMA_TSD_PARAM) : known_(false)
{
  pc_location(addr LIBCWD_COMMA_TSD);
}

/**
 * \brief Destructor.
 */
inline Location::~Location()
{
  clear();
}

inline Location::Location() : function_name_(uninitialized_location), object_file_name_(NULL), known_(false)
{
}

/**
 * \brief Point this location to a different program counter address.
 */
inline void Location::pc_location(void const* addr)
{
  clear();
  LIBCWD_TSD_DECLARATION;
  pc_location(addr LIBCWD_COMMA_TSD);
}

inline bool Location::is_known() const
{
  return known_;
}

inline std::string Location::file() const
{
  LIBCWD_ASSERT(known_);
  return filename_;
}

inline unsigned int Location::line() const
{
  LIBCWD_ASSERT(known_);
  return line_;
}

inline char const* Location::mangled_function_name() const
{
  return function_name_;
}

inline location_format_t location_format(location_format_t format)
{
  LIBCWD_TSD_DECLARATION;
  location_format_t ret = __libcwd_tsd.format;
  __libcwd_tsd.format = format;
  return ret;
}

namespace _private_ {

template <class OSTREAM>
void print_location_on(OSTREAM& os, Location const& location)
{
  if (location.known_)
  {
    LIBCWD_TSD_DECLARATION;
    if ((__libcwd_tsd.format & show_objectfile))
      os << location.object_file_name_->filename() << ':';
    if ((__libcwd_tsd.format & show_function))
      os << location.function_name_ << ':';
    if ((__libcwd_tsd.format & show_path))
      os << location.filepath_.get() << ':' << std::dec << location.line_;
    else
      os << location.filename_ << ':' << std::dec << location.line_;
  }
  else if (location.object_file_name_)
    os << location.object_file_name_->filename() << ':' << location.function_name_;
  else
    os << "<unknown object file> (at " << location.unknown_pc() << ')';
}

} // namespace _private_

#if !(__GNUC__ == 3 && __GNUC_MINOR__ < 4) // See class_location.h
/**
 * \brief Write \a location to ostream \a os.
 * \ingroup group_locations
 *
 * Write the contents of a Location object to an ostream in the form <i>source-file</i>:<i>line-number</i>,
 * or writes <i>objectfile</i>:<i>mangledfuncname</i> when the location is unknown.
 * If the <i>source-file</i>:<i>line-number</i> is known, then it may be prepended by the object file
 * and/or the mangled function name anyway if this was requested through \ref location_format.
 * That function can also be used to cause the <i>source-file</i> to be printed with its full path.
 */
inline std::ostream& operator<<(std::ostream& os, Location const& location)
{
  _private_::print_location_on(os, location);
  return os;
}
#endif

} // namespace libcwd

#endif // CWDEBUG_LOCATION
#endif // LIBCWD_CLASS_LOCATION_INL
