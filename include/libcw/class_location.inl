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

#ifndef LIBCW_CLASS_LOCATION_INL
#define LIBCW_CLASS_LOCATION_INL

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_PRIVATE_ASSERT_H
#include <libcw/private_assert.h>
#endif
#ifndef LIBCW_PRIVATE_THREADING_H
#include <libcw/private_threading.h>
#endif
#ifndef LIBCW_CLASS_LOCATION_H
#include <libcw/class_location.h>
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif
#ifndef LIBCW_IOSTREAM
#define LIBCW_IOSTREAM
#include <iostream>
#endif

namespace libcw {
  namespace debug {

/**
 * \brief Construct a location for address \p addr.
 */
__inline__
location_ct::location_ct(void const* addr) : M_known(false)
{
  LIBCWD_TSD_DECLARATION
  M_pc_location(addr LIBCWD_COMMA_TSD);
}

#if LIBCWD_THREAD_SAFE
/*
 * Construct a location for address addr,
 * taking a thread-specific-data argument.
 */
__inline__
location_ct::location_ct(void const* addr LIBCWD_COMMA_TSD_PARAM) : M_known(false)
{
  M_pc_location(addr LIBCWD_COMMA_TSD);
}
#endif

/**
 * \brief Destructor.
 */
__inline__
location_ct::~location_ct()
{
  clear();
}

__inline__
location_ct::location_ct(void) : M_func("<uninitialized location_ct>"), M_object_file(NULL), M_known(false)
{
}

/**
 * \brief Point this location to a different program counter address.
 */
__inline__
void
location_ct::pc_location(void const* addr)
{
  clear();
  LIBCWD_TSD_DECLARATION
  M_pc_location(addr LIBCWD_COMMA_TSD);
}

__inline__
bool
location_ct::is_known(void) const
{
  return M_known;
}

__inline__
std::string
location_ct::file(void) const
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION
  LIBCWD_ASSERT( !__libcwd_tsd.internal );		// Returning a string, using a userspace allocator!
#endif
  LIBCWD_ASSERT( M_known );
  return M_filename;
}

__inline__
unsigned int
location_ct::line(void) const
{
  LIBCWD_ASSERT( M_known );
  return M_line;
}

__inline__
char const*
location_ct::mangled_function_name(void) const
{
  return M_func;
}

__inline__
void
location_ct::print_filepath_on(std::ostream& os) const
{
  LIBCWD_ASSERT( M_known );
  os << M_filepath.get();
}

__inline__
void
location_ct::print_filename_on(std::ostream& os) const
{
  LIBCWD_ASSERT( M_known );
  os << M_filename;
}

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_LOCATION_INL
