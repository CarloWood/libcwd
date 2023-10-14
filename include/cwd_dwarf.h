// $Header$
//
// @Copyright (C) 2002 - 2007, 2023  Carlo Wood.
//
// pub   dsa3072/C155A4EEE4E527A2 2018-08-16 Carlo Wood (CarloWood on Libera) <carlo@alinoe.com>
// fingerprint: 8020 B266 6305 EE2F D53E  6827 C155 A4EE E4E5 27A2
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef CWD_DWARF_H
#define CWD_DWARF_H

#include "libcwd/config.h"
#if CWDEBUG_LOCATION

#ifndef LIBCW_LIST
#define LIBCW_LIST
#include <list>
#endif
#ifndef LIBCW_SET
#define LIBCW_SET
#include <set>
#endif
#ifndef LIBCWD_PRIVATE_ALLOCATOR_H
#include "libcwd/private_allocator.h"
#endif
#ifndef LIBCWD_CLASS_OBJECT_FILE_H
#include "libcwd/class_object_file.h"
#endif
#ifndef MATCH_H
#include "match.h"
#endif
#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#include "libcwd/private_mutex_instances.h"
#endif
#include "libcwd/debug.h"

#if LIBCWD_THREAD_SAFE
using libcwd::_private_::rwlock_tct;
using libcwd::_private_::mutex_tct;
using libcwd::_private_::object_files_instance;
using libcwd::_private_::dlopen_map_instance;
using libcwd::_private_::dlclose_instance;

#define DWARF_INITIALIZE_LOCK             rwlock_tct<object_files_instance>::initialize()
#define DWARF_ACQUIRE_WRITE_LOCK          rwlock_tct<object_files_instance>::wrlock()
#define DWARF_RELEASE_WRITE_LOCK          rwlock_tct<object_files_instance>::wrunlock()
#define DWARF_ACQUIRE_READ_LOCK           rwlock_tct<object_files_instance>::rdlock()
#define DWARF_ACQUIRE_HP_READ_LOCK        rwlock_tct<object_files_instance>::rdlock(true)
#define DWARF_RELEASE_READ_LOCK           rwlock_tct<object_files_instance>::rdunlock()
#define DWARF_ACQUIRE_READ2WRITE_LOCK     rwlock_tct<object_files_instance>::rd2wrlock()
#define DWARF_ACQUIRE_WRITE2READ_LOCK     rwlock_tct<object_files_instance>::wr2rdlock()
#define DLOPEN_MAP_ACQUIRE_LOCK         mutex_tct<dlopen_map_instance>::lock()
#define DLOPEN_MAP_RELEASE_LOCK         mutex_tct<dlopen_map_instance>::unlock()
#define DLCLOSE_ACQUIRE_LOCK            mutex_tct<dlclose_instance>::lock()
#define DLCLOSE_RELEASE_LOCK            mutex_tct<dlclose_instance>::unlock()
#else // !LIBCWD_THREAD_SAFE
#define DWARF_INITIALIZE_LOCK
#define DWARF_ACQUIRE_WRITE_LOCK
#define DWARF_RELEASE_WRITE_LOCK
#define DWARF_ACQUIRE_READ_LOCK
#define DWARF_ACQUIRE_HP_READ_LOCK
#define DWARF_RELEASE_READ_LOCK
#define DWARF_ACQUIRE_READ2WRITE_LOCK
#define DWARF_ACQUIRE_WRITE2READ_LOCK
#define DLOPEN_MAP_ACQUIRE_LOCK
#define DLOPEN_MAP_RELEASE_LOCK
#define DLCLOSE_ACQUIRE_LOCK
#define DLCLOSE_RELEASE_LOCK
#endif // !LIBCWD_THREAD_SAFE

namespace libcwd {
namespace dwarf {

class objfile_ct;
#if CWDEBUG_ALLOC
using object_files_ct = std::list<objfile_ct*, _private_::object_files_allocator::rebind<objfile_ct*>::other>;
#define LIBCWD_COMMA_ALLOC_OPT(x) , x
#else
using object_files_ct = std::list<objfile_ct*>;
#define LIBCWD_COMMA_ALLOC_OPT(x)
#endif

// All allocations related to objfile_ct must be `internal'.
class objfile_ct
{
private:
  void* M_lbase;
  void const* M_end;
  libcwd::object_file_ct M_object_file;

public:
  objfile_ct(char const* filename, void* base, void const* end);
  bool initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc) LIBCWD_COMMA_TSD_PARAM);
  void deinitialize(LIBCWD_TSD_PARAM);
  void* get_lbase() const { return M_lbase; }
  libcwd::object_file_ct const* get_object_file() const { return &M_object_file; }

  void const* get_start() const { return M_lbase; }
  void const* get_end() const { return M_end; }

private:
  friend object_files_ct const& NEEDS_READ_LOCK_object_files();       // Need access to `ST_list_instance'.
  friend object_files_ct& NEEDS_WRITE_LOCK_object_files();            // Need access to `ST_list_instance'.
  static char ST_list_instance[sizeof(object_files_ct)];
};

inline object_files_ct const&
NEEDS_READ_LOCK_object_files()
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.rdlocked_by1[object_files_instance] == __libcwd_tsd.tid ||
      __libcwd_tsd.rdlocked_by2[object_files_instance] == __libcwd_tsd.tid ||
      _private_::locked_by[object_files_instance] == __libcwd_tsd.tid );
#endif
  return *reinterpret_cast<object_files_ct const*>(objfile_ct::ST_list_instance);
}

inline object_files_ct&
NEEDS_WRITE_LOCK_object_files()
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( _private_::locked_by[object_files_instance] == __libcwd_tsd.tid );
#endif
  return *reinterpret_cast<object_files_ct*>(objfile_ct::ST_list_instance);
}

} // namespace dwarf
} // namespace libcwd

#endif // CWDEBUG_LOCATION
#endif // CWD_DWARF_H
