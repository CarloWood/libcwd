// $Header$
//
// Copyright (C) 2000 - 2003, by
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
#include <libcwd/config.h>
#endif

#if CWDEBUG_LOCATION

#ifndef LIBCW_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif
#ifndef LIBCW_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif
#ifndef LIBCW_CLASS_LOCATION_H
#include <libcwd/class_location.h>
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif

namespace libcw {
  namespace debug {

/**
 * \brief Construct a location for address \p addr.
 */
__inline__
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
__inline__
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
__inline__
location_ct::~location_ct()
{
  clear();
}

__inline__
location_ct::location_ct(void) : M_func(S_uninitialized_location_ct_c), M_object_file(NULL), M_known(false)
#if CWDEBUG_ALLOC
    , M_hide(_private_::new_location)
#endif
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
  LIBCWD_TSD_DECLARATION;
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
  LIBCWD_TSD_DECLARATION;
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

  } // namespace debug
} // namespace libcw

#endif // CWDEBUG_LOCATION
#endif // LIBCW_CLASS_LOCATION_INL
