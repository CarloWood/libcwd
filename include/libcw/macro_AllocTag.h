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

/** \file libcw/macro_AllocTag.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_MACRO_ALLOCTAG_H
#define LIBCW_MACRO_ALLOCTAG_H

#ifndef LIBCW_DEBUG_H
#error "Don't include <libcw/macro_AllocTag.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif

#ifdef DEBUGMALLOC

#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t.
#endif
#ifndef LIBCW_LOCKABLE_AUTO_PTR_H
#include <libcw/lockable_auto_ptr.h>
#endif
#ifndef LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
#include <libcw/private_set_alloc_checking.h>
#endif
#ifndef LIBCW_TYPE_INFO_H
#include <libcw/type_info.h>
#endif
#ifndef LIBCW_PRIVATE_INTERNAL_STRINGSTREAM_H
#include <libcw/private_internal_stringstream.h>
#endif

namespace libcw {
  namespace debug {

/** \addtogroup group_annotation */
/** \{ */

// Undocumented (used inside AllocTag, AllocTag_dynamic_description, AllocTag1 and AllocTag2):
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description);
    // For static descriptions
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, lockable_auto_ptr<char, true> description);
    // For dynamic descriptions
																		    // allocated with new[]
#ifndef DEBUGMALLOCEXTERNALCLINKAGE
extern void register_external_allocation(void const* ptr, size_t size);
#endif

/** \} */ // End of group 'group_annotation'.

  } // namespace debug
} // namespace libcw

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
#define LIBCWD_GETBUFSIZE(buf) buf.rdbuf()->pubseekoff(0, ios::cur, ios::out)
#else
#define LIBCWD_GETBUFSIZE(buf) buf.rdbuf()->pubseekoff(0, ::std::ios_base::cur, ::std::ios_base::out)
#endif

//===================================================================================================
// Macro AllocTag
//

/**
 * \addtogroup group_annotation Allocation Annotation
 * \ingroup book_allocations
 *
 * When memory is allocated, the resulting pointer should be passed to <CODE>AllocTag()</CODE>,
 * passing information that needs to be included in the \ref group_overview "overview of allocated memory".
 * 
 * <b>Example</b>
 * 
 * \code
 * char* buf = (char*)malloc(BUFSIZE);
 * AllocTag(buf, "Temporal buffer");
 * \endcode
 * 
 * would result, in the Allocated memory Overview, in a line like:
 * 
 * \exampleoutput <PRE>
 * malloc    0x804ff38 char [512], (size = 512); Temporal buffer</PRE>
 * \endexampleoutput
 * 
 * And,
 * 
 * \code
 * MyClass* p = new MyClass;
 * AllocTag(p, "This is an example");
 * \endcode
 * 
 * would result in a line like:
 * 
 * \exampleoutput <PRE>
 * 0x804f80c MyClass, (size = 30); This is an example</PRE>
 * \endexampleoutput
 * 
 * While,
 * 
 * \code
 * int* keys = new int [n];
 * AllocTag(keys, "Array with " << n << " keys of " << my_get_name());
 * \endcode
 * 
 * would result in a line like:
 * 
 * \exampleoutput <PRE>
 * new[]     0x804f6b8 int [5], (size = 20); Array with 5 keys of Mr.Frubal.</PRE>
 * \endexampleoutput
 * 
 * It is allowed to call <CODE>AllocTag()</CODE> more then once for the same pointer, in that case the last
 * call will simply override the previous calls.&nbsp;
 * It is also allowed to call <CODE>AllocTag()</CODE> for a pointer that is not allocated at all,
 * in which case it is ignored.&nbsp;
 * The main reason for this is to allow the use of <CODE>AllocTag(this, "description")</CODE> in the constructor of any object,
 * even in relation with (multiple) inheritance.
 * 
 * It is also allowed to call <CODE>AllocTag()</CODE> with a pointer which points inside an allocated
 * memory block, which has the same effect as using the pointer to the start of it.
 */

/** \addtogroup group_annotation */
/** \{ */

/**
 * \brief Annotate <I>type</I> of \a p.
 */
#define AllocTag1(p) ::libcw::debug::\
    set_alloc_label(p, ::libcw::debug::type_info_of(p), (char const*)NULL)
/**
 * \brief Annotate <I>type</I> of \a p with string literal \a desc as description.
 */
#define AllocTag2(p, desc) ::libcw::debug::\
    set_alloc_label(p, ::libcw::debug::type_info_of(p), const_cast<char const*>(desc))

#ifdef _REENTRANT
#define LIBCWD_LOCK_desc__if_still_NULL_then ::libcw::debug::_private_::\
    mutex_tct< ::libcw::debug::_private_::alloc_tag_desc_instance>::lock(); \
    if (!desc)
#define LIBCWD_UNLOCK_desc ::libcw::debug::_private_::\
    mutex_tct< ::libcw::debug::_private_::alloc_tag_desc_instance>::unlock();
#else // !_REENTRANT
#define LIBCWD_LOCK_desc__if_still_NULL_then
#define LIBCWD_UNLOCK_desc
#endif // !_REENTRANT

/**
 * \brief Annotate <I>type</I> of \a p with a static description.
 */
#define AllocTag(p, x) \
    do { \
      static char* desc; /* MT-safe */ \
      if (!desc) { \
	LIBCWD_TSD_DECLARATION \
	::libcw::debug::_private_::set_alloc_checking_off(LIBCWD_TSD); \
	LIBCWD_LOCK_desc__if_still_NULL_then \
	if (1) \
	{ \
	  ::libcw::debug::_private_::internal_stringstream buf; \
	  buf << x << ::std::ends; \
	  size_t size = LIBCWD_GETBUFSIZE(buf); \
	  desc = new char [size]; /* This is never deleted anymore */ \
	  buf.rdbuf()->sgetn(desc, size); \
	} \
	LIBCWD_UNLOCK_desc \
	::libcw::debug::_private_::set_alloc_checking_on(LIBCWD_TSD); \
      } \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), desc); \
    } while(0)

/**
 * \brief Annotate <I>type</I> of \a p with a dynamic description.
 */
#define AllocTag_dynamic_description(p, x) \
    do { \
      char* desc; \
      LIBCWD_TSD_DECLARATION \
      ::libcw::debug::_private_::set_alloc_checking_off(LIBCWD_TSD); \
      if (1) \
      { \
	::libcw::debug::_private_::internal_stringstream buf; \
	buf << x << ::std::ends; \
	size_t size = LIBCWD_GETBUFSIZE(buf); \
	desc = new char [size]; \
	buf.rdbuf()->sgetn(desc, size); \
      } \
      ::libcw::debug::_private:::set_alloc_checking_on(LIBCWD_TSD); \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), \
	  			      ::libcw::lockable_auto_ptr<char, true>(desc)); \
    } while(0)

template<typename TYPE>
#ifndef __FreeBSD__
  // There is a bug in g++ that causes the wrong line number information to be generated
  // when this function is inline.  I was able to use heuristics to work around that for
  // STABS, but not for DWARF-2 (the default of FreeBSD).
  // See http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view%20audit-trail&database=gcc&pr=5271
  __inline__
#endif
  TYPE*
  __libcwd_allocCatcher(TYPE* new_ptr)
  {
    AllocTag1(new_ptr);
    return new_ptr;
  };

/**
 * \brief Like operator \c new, but automatically annotates the \em type of \a x.
 */
#define NEW(x) __libcwd_allocCatcher(new x)

#ifndef DEBUGMALLOCEXTERNALCLINKAGE
/**
 * \brief Register an externally allocated memory block at position \a ptr with size \a size.
 */
#define RegisterExternalAlloc(p, s) ::libcw::debug::register_external_allocation(p, s)
#endif

/** \} */ // End of group 'group_annotation'.

#else // !DEBUGMALLOC

/** \addtogroup group_annotation */
/** \{ */

#define AllocTag(p, x)
#define AllocTag_dynamic_description(p, x)
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define NEW(x) new x
#ifndef DEBUGMALLOCEXTERNALCLINKAGE
#define RegisterExternalAlloc(p, s)
#endif

/** \} */ // End of group 'group_annotation'.

#endif // !DEBUGMALLOC

#endif // LIBCW_MACRO_ALLOCTAG_H

