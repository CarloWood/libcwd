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

#include "sys.h"
#include <libcwd/config.h>
#if CWDEBUG_ALLOC
#include "cwd_debug.h"
#include "cwd_bfd.h"
#ifndef LIBCWD_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif
#ifndef LIBCWD_CLASS_ALLOC_FILTER_H
#include <libcwd/class_alloc_filter.h>
#endif

namespace libcwd {

#if LIBCWD_THREAD_SAFE
#define ACQUIRE_LISTALLOC_LOCK mutex_tct<list_allocations_instance>::lock()
#define RELEASE_LISTALLOC_LOCK mutex_tct<list_allocations_instance>::unlock()
using _private_::mutex_tct;

#if __GNUC_MINOR__ != 5
using _private_::list_allocations_instance;
#else
// gcc version 3.5.0 20040420 (experimental) ICEs on the above.
namespace workaround_20040420 = ::libcwd::_private_::workaround_20040420;
#define list_allocations_instance workaround_20040420::list_allocations_instance
#endif

#else
#define ACQUIRE_LISTALLOC_LOCK
#define RELEASE_LISTALLOC_LOCK
#endif

#if CWDEBUG_LOCATION
int alloc_filter_ct::S_id = 0;			// Id of alloc_filter_ct object that is currently synchronized with the other
						// global variables of the critical area(s) of list_allocations_instance.
int alloc_filter_ct::S_next_id = 0;		// Id of the last alloc_filter_ct object that was created.
#endif

// Use '1' instead of '0' because 0 is more likely to be used by the user
// as an extreme but real limit.
struct timeval const alloc_filter_ct::no_time_limit = { 1, 0 };

void alloc_filter_ct::set_flags(alloc_format_t flags)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  M_flags &= ~format_mask;
  M_flags |= flags;
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
}

alloc_format_t alloc_filter_ct::get_flags(void) const
{
  // Makes little sense to add locking here (alloc_format_t is an atomic type).
  return (M_flags & format_mask);
}

void alloc_filter_ct::set_time_interval(struct timeval const& start, struct timeval const& end)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  M_start = start;
  M_end = end;
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
}

struct timeval alloc_filter_ct::get_time_start(void) const
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  struct timeval res;
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  res = M_start;
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
  return res;
}

struct timeval alloc_filter_ct::get_time_end(void) const
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  struct timeval res;
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  res = M_end;
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
  return res;
}

#if CWDEBUG_LOCATION
std::vector<std::string> alloc_filter_ct::get_objectfile_list(void) const
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  std::vector<std::string> res;
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  for(vector_type::const_iterator iter = M_objectfile_masks.begin(); iter != M_objectfile_masks.end(); ++iter)
    res.push_back(std::string(iter->data(), iter->length()));
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
  return res;
}

std::vector<std::string> alloc_filter_ct::get_sourcefile_list(void) const
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  std::vector<std::string> res;
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  for(vector_type::const_iterator iter = M_sourcefile_masks.begin(); iter != M_sourcefile_masks.end(); ++iter)
    res.push_back(std::string(iter->data(), iter->length()));
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
  return res;
}

std::vector<std::pair<std::string, std::string> > alloc_filter_ct::get_function_list(void) const
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  std::vector<std::pair<std::string, std::string> > res;
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  for(vector_pair_type::const_iterator iter = M_function_masks.begin(); iter != M_function_masks.end(); ++iter)
    res.push_back(std::pair<std::string, std::string>(std::string(iter->first.data(), iter->first.length()),
                                                      std::string(iter->second.data(), iter->second.length())));
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
  return res;
}

void alloc_filter_ct::hide_objectfiles_matching(std::vector<std::string> const& masks)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  M_objectfile_masks.clear();
  for(std::vector<std::string>::const_iterator iter = masks.begin(); iter != masks.end(); ++iter)
    M_objectfile_masks.push_back(string_type(iter->data(), iter->length()));
  S_id = -1;			// Force resynchronization.
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
}

void alloc_filter_ct::hide_sourcefiles_matching(std::vector<std::string> const& masks)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  M_sourcefile_masks.clear();
  for(std::vector<std::string>::const_iterator iter = masks.begin(); iter != masks.end(); ++iter)
    M_sourcefile_masks.push_back(string_type(iter->data(), iter->length()));
  S_id = -1;			// Force resynchronization.
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
}

void alloc_filter_ct::hide_functions_matching(std::vector<std::pair<std::string, std::string> > const& masks)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  M_sourcefile_masks.clear();
  for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = masks.begin(); iter != masks.end(); ++iter)
    M_function_masks.push_back(std::pair<string_type, string_type>
        (string_type(iter->first.data(), iter->first.length()), string_type(iter->second.data(), iter->second.length())));
  S_id = -1;			// Force resynchronization.
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
}

_private_::hidden_st alloc_filter_ct::check_hide(char const* filepath) const
{
  for (vector_type::const_iterator iter(M_sourcefile_masks.begin()); iter != M_sourcefile_masks.end(); ++iter)
    if (_private_::match(iter->data(), iter->length(), filepath))
      return _private_::filtered_location;
  return _private_::unfiltered_location;
}

_private_::hidden_st alloc_filter_ct::check_hide(object_file_ct const* object_file, char const* mangled_function_name) const
{
  char const* file_path = object_file->filepath();
  char const* file_name = object_file->filename();
  for (vector_pair_type::const_iterator iter = M_function_masks.begin(); iter != M_function_masks.end(); ++iter)
  {
    char const* objectfile_mask = iter->first.data();
    size_t objectfile_mask_length = iter->first.length();
    char c;
    if ((objectfile_mask_length == 0 ||
          (((c = *objectfile_mask) == '/' || c == '*') &&
	    _private_::match(objectfile_mask, objectfile_mask_length, file_path)) ||
	  (!(c == '/' || c == '*') &&
	    _private_::match(objectfile_mask, objectfile_mask_length, file_name))) &&
        (iter->second.length() == 0 ||
          _private_::match(iter->second.data(), iter->second.length(), mangled_function_name)))
      return _private_::filtered_location;
  }
  return _private_::unfiltered_location;
}
#endif // CWDEBUG_LOCATION

#if CWDEBUG_LOCATION
void alloc_filter_ct::M_synchronize(void) const
{
#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUG
  LIBCWD_ASSERT( _private_::is_locked(list_allocations_instance) );
#endif
  BFD_ACQUIRE_WRITE_LOCK;
  // First clear the list, unhiding everything.
  for (cwbfd::object_files_ct::iterator iter = cwbfd::NEEDS_WRITE_LOCK_object_files().begin();
       iter != cwbfd::NEEDS_WRITE_LOCK_object_files().end();
       ++iter)
    (*iter)->get_object_file()->M_hide = false;
  // Next hide what matches.
  if (!M_objectfile_masks.empty())
  {
    for (cwbfd::object_files_ct::iterator iter = cwbfd::NEEDS_WRITE_LOCK_object_files().begin();
	 iter != cwbfd::NEEDS_WRITE_LOCK_object_files().end();
	 ++iter)
    {
      for (vector_type::const_iterator iter2(M_objectfile_masks.begin()); iter2 != M_objectfile_masks.end(); ++iter2)
	if (_private_::match((*iter2).data(), (*iter2).length(), (*iter)->get_object_file()->M_filename))
	{
	  (*iter)->get_object_file()->M_hide = true;
	  break;
	}
    }
  }
  BFD_RELEASE_WRITE_LOCK;
  M_synchronize_locations();
  S_id = M_id;
}
#endif

alloc_filter_ct::alloc_filter_ct(alloc_format_t flags) : M_flags(flags & format_mask), M_start(no_time_limit), M_end(no_time_limit)
{
#if CWDEBUG_LOCATION
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
  ACQUIRE_LISTALLOC_LOCK;
  M_id = ++S_next_id;
  RELEASE_LISTALLOC_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);
#endif
}

} // namespace libcwd

#endif // CWDEBUG_ALLOC
