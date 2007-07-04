// $Header$
//
// Copyright (C) 2001 - 2007, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef ELFXX_H
#define ELFXX_H

#ifndef LIBCW_INTTYPES_H
#define LIBCW_INTTYPES_H
#include <inttypes.h>
#endif
#ifndef LIBCW_ELF_H
#define LIBCW_ELF_H
#include <elf.h>
#endif

namespace libcwd {

namespace cwbfd {
class bfile_ct;
static int const BSF_LOCAL             = 0x1;
static int const BSF_GLOBAL            = 0x2;
static int const BSF_DEBUGGING         = 0x8;
static int const BSF_FUNCTION          = 0x10;
static int const BSF_WEAK              = 0x80;
static int const BSF_SECTION_SYM       = 0x100;
static int const BSF_OLD_COMMON        = 0x200;
static int const BSF_NOT_AT_END        = 0x400;
static int const BSF_CONSTRUCTOR       = 0x800;
static int const BSF_WARNING           = 0x1000;
static int const BSF_INDIRECT          = 0x2000;
static int const BSF_FILE              = 0x4000;
static int const BSF_DYNAMIC           = 0x8000;
static int const BSF_OBJECT            = 0x10000;
} // namespace cwbfd

namespace elfxx {

#ifdef __x86_64__
typedef Elf64_Addr Elfxx_Addr;
typedef Elf64_Off Elfxx_Off;
typedef Elf64_Word Elfxx_Word;
#else
typedef ELf32_Addr Elfxx_Addr;
typedef Elf32_Off Elfxx_Off;
typedef Elf32_Word Elfxx_Word;
#endif

struct asection_st;
struct bfd_st;

struct asymbol_st {
  bfd_st* bfd_ptr;
  asection_st const* section;
  Elfxx_Off value;
  size_t size;
  Elfxx_Word flags;
  char const* name;
};

struct asection_st {
  Elfxx_Addr vma;
  char const* name;
  Elfxx_Word M_size;
};

struct bfd_st {
  _private_::internal_string filename_str;
  cwbfd::bfile_ct const* object_file;
  bool cacheable;
  bool M_has_syms;
  bool M_is_stripped;
  size_t M_s_end_offset;
public:
  bfd_st(void) : M_has_syms(false), M_is_stripped(true) { }
  virtual ~bfd_st() { }
public:
  static bfd_st* openr(char const* file_name);
  virtual void close(void) = 0;
  virtual bool check_format(void) const = 0;
  virtual long get_symtab_upper_bound(void) = 0;
  virtual long canonicalize_symtab(asymbol_st**) = 0;
  virtual void find_nearest_line(asymbol_st const*, Elfxx_Addr, char const**, char const**, unsigned int* LIBCWD_COMMA_TSD_PARAM) = 0;
  bool has_syms(void) const { return M_has_syms; }
  bool is_stripped(void) const { return M_is_stripped; }
};

extern asection_st const* const absolute_section_c;

} // namespace elfxx

} // namespace libcwd

#endif // ELFXX_H
