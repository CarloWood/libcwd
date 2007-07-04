// $Header$
//
// Copyright (C) 2002 - 2007, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef CWD_BFD_H
#define CWD_BFD_H

#include <libcwd/config.h>
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
#include <libcwd/private_allocator.h>
#endif
#ifndef ELFXX_H
#include "elfxx.h"
#endif
#ifndef LIBCWD_CLASS_OBJECT_FILE_H
#include <libcwd/class_object_file.h>
#endif
#ifndef MATCH_H
#include "match.h"
#endif
#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#include <libcwd/private_mutex_instances.h>
#endif

#if LIBCWD_THREAD_SAFE
using libcwd::_private_::rwlock_tct;
using libcwd::_private_::mutex_tct;
using libcwd::_private_::object_files_instance;
using libcwd::_private_::dlopen_map_instance;
using libcwd::_private_::dlclose_instance;

#define BFD_INITIALIZE_LOCK             rwlock_tct<object_files_instance>::initialize()
#define BFD_ACQUIRE_WRITE_LOCK          rwlock_tct<object_files_instance>::wrlock()
#define BFD_RELEASE_WRITE_LOCK          rwlock_tct<object_files_instance>::wrunlock()
#define BFD_ACQUIRE_READ_LOCK           rwlock_tct<object_files_instance>::rdlock()
#define BFD_ACQUIRE_HP_READ_LOCK        rwlock_tct<object_files_instance>::rdlock(true)
#define BFD_RELEASE_READ_LOCK           rwlock_tct<object_files_instance>::rdunlock()
#define BFD_ACQUIRE_READ2WRITE_LOCK     rwlock_tct<object_files_instance>::rd2wrlock()
#define BFD_ACQUIRE_WRITE2READ_LOCK     rwlock_tct<object_files_instance>::wr2rdlock()
#define DLOPEN_MAP_ACQUIRE_LOCK         mutex_tct<dlopen_map_instance>::lock()
#define DLOPEN_MAP_RELEASE_LOCK         mutex_tct<dlopen_map_instance>::unlock()
#define DLCLOSE_ACQUIRE_LOCK            mutex_tct<dlclose_instance>::lock()
#define DLCLOSE_RELEASE_LOCK            mutex_tct<dlclose_instance>::unlock()
#else // !LIBCWD_THREAD_SAFE
#define BFD_INITIALIZE_LOCK
#define BFD_ACQUIRE_WRITE_LOCK
#define BFD_RELEASE_WRITE_LOCK
#define BFD_ACQUIRE_READ_LOCK
#define BFD_ACQUIRE_HP_READ_LOCK
#define BFD_RELEASE_READ_LOCK
#define BFD_ACQUIRE_READ2WRITE_LOCK
#define BFD_ACQUIRE_WRITE2READ_LOCK
#define DLOPEN_MAP_ACQUIRE_LOCK
#define DLOPEN_MAP_RELEASE_LOCK
#define DLCLOSE_ACQUIRE_LOCK
#define DLCLOSE_RELEASE_LOCK
#endif // !LIBCWD_THREAD_SAFE

namespace libcwd {
  namespace cwbfd {

class symbol_ct {
private:
  elfxx::asymbol_st* symbol;
public:
  symbol_ct(elfxx::asymbol_st* p) : symbol(p) { }
  elfxx::asymbol_st const* get_symbol(void) const { return symbol; }
  bool operator==(symbol_ct const&) const { DoutFatal(dc::core, "Calling operator=="); }
  friend struct symbol_key_greater;
};

struct symbol_key_greater {
  // Returns true when the start of a lays beyond the end of b (ie, no overlap).
  bool operator()(symbol_ct const& a, symbol_ct const& b) const;
};

#if CWDEBUG_ALLOC
typedef std::set<symbol_ct, symbol_key_greater, _private_::object_files_allocator::rebind<symbol_ct>::other> function_symbols_ct;
#else
typedef std::set<symbol_ct, symbol_key_greater> function_symbols_ct;
#endif

class bfile_ct;
#if CWDEBUG_ALLOC
typedef std::list<bfile_ct*, _private_::object_files_allocator::rebind<bfile_ct*>::other> object_files_ct;
#define LIBCWD_COMMA_ALLOC_OPT(x) , x
#else
typedef std::list<bfile_ct*> object_files_ct;
#define LIBCWD_COMMA_ALLOC_OPT(x)
#endif

class bfile_ct {                                  // All allocations related to bfile_ct must be `internal'.
private:
  elfxx::bfd_st* M_abfd;
  void* M_lbase;			// The 'load address', or 0 for the executable.
  void const* M_start;			// The start address of the first symbol.
  void const* M_start_last_symbol;	// The start address of the last symbol, or 0 if not relevant.
  size_t M_size;			// The difference between M_start and the end of the last symbol.
  elfxx::asymbol_st** M_symbol_table;
  long M_number_of_symbols;
  function_symbols_ct M_function_symbols;
  libcwd::object_file_ct M_object_file;
public:
  bfile_ct(char const* filename, void* base);
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
  bool initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc), bool is_libstdcpp LIBCWD_COMMA_TSD_PARAM);
#else
  bool initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc) LIBCWD_COMMA_TSD_PARAM);
#endif
  void deinitialize(LIBCWD_TSD_PARAM);

  elfxx::bfd_st* get_bfd(void) const { return M_abfd; }
  void* get_lbase(void) const { return M_lbase; }
  void const* get_start(void) const { return M_start; }
  size_t size(void) const { return M_size; }
  elfxx::asymbol_st** get_symbol_table(void) const { return M_symbol_table; }
  long get_number_of_symbols(void) const { return M_number_of_symbols; }
  libcwd::object_file_ct const* get_object_file(void) const { return &M_object_file; }
  function_symbols_ct& get_function_symbols(void) { return M_function_symbols; }
  function_symbols_ct const& get_function_symbols(void) const { return M_function_symbols; }
private:
  friend object_files_ct const& NEEDS_READ_LOCK_object_files(void);       // Need access to `ST_list_instance'.
  friend object_files_ct& NEEDS_WRITE_LOCK_object_files(void);            // Need access to `ST_list_instance'.
  static char ST_list_instance[sizeof(object_files_ct)];
};

inline object_files_ct const&
NEEDS_READ_LOCK_object_files(void)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.rdlocked_by1[object_files_instance] == __libcwd_tsd.tid ||
      __libcwd_tsd.rdlocked_by2[object_files_instance] == __libcwd_tsd.tid ||
      _private_::locked_by[object_files_instance] == __libcwd_tsd.tid );
#endif
  return *reinterpret_cast<object_files_ct const*>(bfile_ct::ST_list_instance);
}

inline object_files_ct&
NEEDS_WRITE_LOCK_object_files(void)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( _private_::locked_by[object_files_instance] == __libcwd_tsd.tid );
#endif
  return *reinterpret_cast<object_files_ct*>(bfile_ct::ST_list_instance);
}

inline char const*
symbol_start_addr(elfxx::asymbol_st const* s)
{
  return s->value + s->section->vma
      + reinterpret_cast<char const*>(s->bfd_ptr->object_file->get_lbase());
}

// cwbfd::
inline size_t symbol_size(elfxx::asymbol_st const* s)
{
  return static_cast<size_t>(s->size);
}

// cwbfd::
inline size_t& symbol_size(elfxx::asymbol_st* s)
{
  return s->size;
}

  } // namespace cwbfd

} // namespace libcwd

#endif // CWDEBUG_LOCATION
#endif // CWD_BFD_H

