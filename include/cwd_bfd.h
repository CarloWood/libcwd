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
#ifndef ELF32_H
#include "elf32.h"
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

#if __GNUC_MINOR__ != 5
using libcwd::_private_::object_files_instance;
using libcwd::_private_::dlopen_map_instance;
using libcwd::_private_::dlclose_instance;
#else
// gcc version 3.5.0 20040420 (experimental) ICEs on the above.
namespace libcwd { namespace _private_ { namespace workaround_20040420 {
  static mutex_instance_nt const object_files_instance = _private_::object_files_instance;
  static mutex_instance_nt const dlopen_map_instance = _private_::dlopen_map_instance;
  static mutex_instance_nt const dlclose_instance = _private_::dlclose_instance;
  static mutex_instance_nt const list_allocations_instance = _private_::list_allocations_instance;
}}}
namespace workaround_20040420 = libcwd::_private_::workaround_20040420;
#define object_files_instance workaround_20040420::object_files_instance
#define dlopen_map_instance workaround_20040420::dlopen_map_instance
#define dlclose_instance workaround_20040420::dlclose_instance
#endif

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

#if !CWDEBUG_LIBBFD
typedef char* PTR;
typedef elf32::bfd_st bfd;
typedef elf32::Elf32_Addr bfd_vma;
typedef elf32::asection_st asection;
typedef elf32::asymbol_st asymbol;
#endif

class symbol_ct {
private:
  asymbol* symbol;
  bool defined;
public:
  symbol_ct(asymbol* p, bool def) : symbol(p), defined(def) { }
  asymbol const* get_symbol(void) const { return symbol; }
  bool is_defined(void) const { return defined; }
  bool operator==(symbol_ct const&) const { DoutFatal(dc::core, "Calling operator=="); }
  friend struct symbol_key_greater;
};

struct symbol_key_greater {
  // Returns true when the start of a lays behond the end of b (ie, no overlap).
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
#else
typedef std::list<bfile_ct*> object_files_ct;
#endif

class bfile_ct {                                  // All allocations related to bfile_ct must be `internal'.
private:
  bfd* M_abfd;
  void* M_lbase;
  size_t M_size;
  asymbol** M_symbol_table;
  long M_number_of_symbols;
  function_symbols_ct M_function_symbols;
  libcwd::object_file_ct M_object_file;
public:
  bfile_ct(char const* filename, void* base);
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ >= 4
  void initialize(char const* filename, void* base, bool is_libstdcpp LIBCWD_COMMA_TSD_PARAM);
#else
  void initialize(char const* filename, void* base LIBCWD_COMMA_TSD_PARAM);
#endif
  void deinitialize(LIBCWD_TSD_PARAM);

  bfd* get_bfd(void) const { return M_abfd; }
  void* const get_lbase(void) const { return M_lbase; }
  size_t size(void) const { return M_size; }
  asymbol** get_symbol_table(void) const { return M_symbol_table; }
  long get_number_of_symbols(void) const { return M_number_of_symbols; }
  libcwd::object_file_ct const* get_object_file(void) const { return &M_object_file; }
  libcwd::object_file_ct* get_object_file(void) { return &M_object_file; }
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

#if !CWDEBUG_LIBBFD
inline asection const* bfd_get_section(asymbol const* s) { return s->section; }
inline bfd*& bfd_asymbol_bfd(asymbol* s) { return s->bfd_ptr; }
inline bfd* bfd_asymbol_bfd(asymbol const* s) { return s->bfd_ptr; }

#ifdef PTR
      typedef const PTR addr_const_ptr_t;       // Warning: PTR is a macro, must put `const' in front of it
#else
      typedef char const* addr_const_ptr_t;
#endif

inline addr_const_ptr_t
symbol_start_addr(asymbol const* s)
{
  return s->value + bfd_get_section(s)->offset
      + reinterpret_cast<char const*>(reinterpret_cast<bfile_ct const*>(bfd_asymbol_bfd(s)->usrdata)->get_lbase());
}

// cwbfd::
inline size_t symbol_size(asymbol const* s)
{
  return reinterpret_cast<size_t>(s->udata.p);
}

// cwbfd::
inline size_t& symbol_size(asymbol* s)
{
  return *reinterpret_cast<size_t*>(&s->udata.p);
}
#endif

  } // namespace cwbfd

} // namespace libcwd

#endif // CWDEBUG_LOCATION
#endif // CWD_BFD_H

