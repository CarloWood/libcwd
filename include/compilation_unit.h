// $Header$
//
// Copyright (C) 2003 - 2007, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef COMPILATION_UNIT_H
#define COMPILATION_UNIT_H

#include "libcwd/private_internal_string.h"
#include "libcwd/class_function.h"
#ifndef LIBCW_VECTOR
#define LIBCW_VECTOR
#include <vector>
#endif
#ifndef LIBCW_INTTYPES_H
#define LIBCW_INTTYPES_H
#include <inttypes.h>
#endif

namespace libcwd {
  namespace _private_ {

class compilation_unit_ct {
private:
  void const* M_lowpc;				// First byte of compilation unit.
  void const* M_highpc;				// One past last byte of compilation unit.
  internal_string M_compilation_directory;	// Working directory of compiler when compiling this compilation unit.
  internal_string M_source_file;		// Name of main source file producing this compilation unit.
  FunctionRootsMap M_function_roots;		// List of function roots.
public:
  // Dirty initialization - the idea is to decouple us here from the rest of elfxx.cc and bfd.cc.
  // These four are called once, from elfxx.cc.
#if CWDEBUG_ALLOC
  typedef std::basic_string<char, std::char_traits<char>, _private_::object_files_allocator> object_files_string;
#else
  typedef std::string object_files_string;
#endif
#if defined(__x86_64__) || defined(__sparc64) || defined(__ia64__)
  typedef uint64_t Elfxx_Addr;        // Elf64_Addr.
#else
  typedef uint32_t Elfxx_Addr;        // Elf32_Addr.
#endif
  void set_lowpc(Elfxx_Addr lowpc)
      { M_lowpc = reinterpret_cast<void const*>(lowpc); }
  void set_highpc(Elfxx_Addr highpc)
      { M_highpc = reinterpret_cast<void const*>(highpc); }
  void set_compilation_directory(object_files_string const& comp_dir)
      { M_compilation_directory.assign(comp_dir.data(), comp_dir.size()); }
  void set_source_file(object_files_string const& source_file)
      { M_source_file.assign(source_file.data(), source_file.size()); }
  FunctionRootsMap& function_roots() { return M_function_roots; }
  FunctionRootsMap const& function_roots() const { return M_function_roots; }

  void const* get_lowpc() const { return M_lowpc; }
  void const* get_highpc() const { return M_highpc; }
  internal_string get_source_file() const { return M_source_file; }
  internal_string get_compilation_directory() const { return M_compilation_directory; }
  FunctionRootsMap const& get_function_roots() const { return M_function_roots; }
};

// This type is added to class bfile_ct.
#if CWDEBUG_ALLOC
typedef std::vector<compilation_unit_ct, object_files_allocator::rebind<compilation_unit_ct>::other> compilation_units_vector_ct;
#else
typedef std::vector<compilation_unit_ct> compilation_units_vector_ct;
#endif

  } // namespace _private_
} // namespace libcwd

#endif // COMPILATION_UNIT_H

