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

/** \file libcwd/macro_AllocTag.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_MACRO_ALLOCTAG_H
#define LIBCWD_MACRO_ALLOCTAG_H

#ifndef LIBCWD_DEBUG_H
#error "Don't include <libcwd/macro_AllocTag.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif

#if CWDEBUG_ALLOC

#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t.
#endif
#ifndef LIBCWD_SMART_PTR_H
#include <libcwd/smart_ptr.h>
#endif
#ifndef LIBCWD_PRIVATE_SET_ALLOC_CHECKING_H
#include <libcwd/private_set_alloc_checking.h>
#endif
#ifndef LIBCWD_TYPE_INFO_H
#include <libcwd/type_info.h>
#endif
#ifndef LIBCWD_PRIVATE_INTERNAL_STRINGSTREAM_H
#include <libcwd/private_internal_stringstream.h>
#endif

namespace libcw {
  namespace debug {

/** \addtogroup group_annotation */
/** \{ */

// Undocumented (used inside AllocTag, AllocTag_dynamic_description, AllocTag1 and AllocTag2):
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description LIBCWD_COMMA_TSD_PARAM);
    // For static descriptions
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, _private_::smart_ptr description LIBCWD_COMMA_TSD_PARAM);
    // For dynamic descriptions
    // allocated with new[]
#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
extern void register_external_allocation(void const* ptr, size_t size);
#endif

/** \} */ // End of group 'group_annotation'.

  } // namespace debug
} // namespace libcw

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
#define AllocTag1(p) \
    do { \
      LIBCWD_TSD_DECLARATION; \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), (char const*)NULL LIBCWD_COMMA_TSD); \
    } while(0)
/**
 * \brief Annotate <I>type</I> of \a p with string literal \a desc as description.
 */
#define AllocTag2(p, desc) \
    do { \
      LIBCWD_TSD_DECLARATION; \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), const_cast<char const*>(desc) LIBCWD_COMMA_TSD); \
    } while(0)

#if LIBCWD_THREAD_SAFE
#define LIBCWD_ALLOCTAG_LOCK \
    if (!WS_desc) \
    { \
      static pthread_mutex_t WS_desc_mutex = PTHREAD_MUTEX_INITIALIZER; \
      pthread_mutex_lock(&WS_desc_mutex);
#define LIBCWD_ALLOCTAG_UNLOCK \
      pthread_mutex_unlock(&WS_desc_mutex); \
    }
#else
#define LIBCWD_ALLOCTAG_LOCK
#define LIBCWD_ALLOCTAG_UNLOCK
#endif

/**
 * \brief Annotate <I>type</I> of \a p with a static description.
 */
#define AllocTag(p, x) \
    do { \
      LIBCWD_TSD_DECLARATION; \
      static char* WS_desc; \
      LIBCWD_ALLOCTAG_LOCK; \
      if (!WS_desc) { \
	++LIBCWD_DO_TSD_MEMBER_OFF(::libcw::debug::libcw_do); \
	if (1) \
	{ \
	  ::libcw::debug::_private_::auto_internal_stringstream buf; \
	  buf << x << ::std::ends; \
	  ::std::streampos pos = buf.rdbuf()->pubseekoff(0, ::std::ios_base::cur, ::std::ios_base::out); \
	  size_t size = pos - ::std::streampos(0); \
	  ::libcw::debug::_private_::set_alloc_checking_off(LIBCWD_TSD); \
	  WS_desc = new char [size]; /* This is never deleted anymore */ \
	  ::libcw::debug::_private_::set_alloc_checking_on(LIBCWD_TSD); \
	  buf.rdbuf()->sgetn(WS_desc, size); \
	} \
	--LIBCWD_DO_TSD_MEMBER_OFF(::libcw::debug::libcw_do); \
      } \
      LIBCWD_ALLOCTAG_UNLOCK; \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), WS_desc LIBCWD_COMMA_TSD); \
    } while(0)

/**
 * \brief Annotate <I>type</I> of \a p with a dynamic description.
 */
#define AllocTag_dynamic_description(p, x) \
    do { \
      char* desc; \
      LIBCWD_TSD_DECLARATION; \
      ++LIBCWD_DO_TSD_MEMBER_OFF(::libcw::debug::libcw_do); \
      if (1) \
      { \
	::libcw::debug::_private_::auto_internal_stringstream buf; \
	buf << x << ::std::ends; \
	::std::streampos pos = buf.rdbuf()->pubseekoff(0, ::std::ios_base::cur, ::std::ios_base::out); \
	size_t size = pos - ::std::streampos(0); \
	::libcw::debug::_private_::set_alloc_checking_off(LIBCWD_TSD); \
	desc = new char [size]; \
	::libcw::debug::_private_::set_alloc_checking_on(LIBCWD_TSD); \
	buf.rdbuf()->sgetn(desc, size); \
      } \
      --LIBCWD_DO_TSD_MEMBER_OFF(::libcw::debug::libcw_do); \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), \
	  			      ::libcw::debug::_private_::smart_ptr(desc) LIBCWD_COMMA_TSD); \
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

#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
/**
 * \brief Register an externally allocated memory block at position \a ptr with size \a size.
 */
#define RegisterExternalAlloc(p, s) ::libcw::debug::register_external_allocation(p, s)
#endif

/** \} */ // End of group 'group_annotation'.

#else // !CWDEBUG_ALLOC

/** \addtogroup group_annotation */
/** \{ */

#define AllocTag(p, x)
#define AllocTag_dynamic_description(p, x)
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define NEW(x) new x
#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
#define RegisterExternalAlloc(p, s)
#endif

/** \} */ // End of group 'group_annotation'.

#endif // !CWDEBUG_ALLOC

#endif // LIBCWD_MACRO_ALLOCTAG_H

