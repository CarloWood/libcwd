// $Header$
//
// Copyright (C) 2001, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

//
// This file contains code that reads the symbol table and debug
// information from (ELF) object files.
//
// The information needed to write this file has been obtained from
// a file called 'ELF.doc.tar.gz'.  You can get this file from the
// net, for example: ftp://ftp.metalab.unc.edu/pub/Linux/GCC/ELF.doc.tar.gz.
// Special thanks goes to the FSF who released libbfd under the GPL
// instead of the LGPL, making it impossible for me to use their
// library.
//

#include <libcw/sys.h>
#include <inttypes.h>	// ISO C99 header, needed for int32_t etc.
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif
#include <fstream>
#include <libcw/debug.h>
#include <libcw/elf32.h>

RCSTAG_CC("$Id$")

namespace libcw {
  namespace debug {

namespace elf32 {

//-------------------------------------------------------------------------------------------------------------------------------------------
// ELF header table

// Figure 1-3: ELF Header.

int const EI_NIDENT = 16;

struct Elf32_Ehdr {
  unsigned char		e_ident[EI_NIDENT];	// ELF Identification entry.
  Elf32_Half		e_type;			// Object file type.
  Elf32_Half		e_machine;		// Architecture type.
  Elf32_Word		e_version;		// Object file version.
  Elf32_Addr		e_entry;		// Virtual address of entry point.
  Elf32_Off		e_phoff;		// Program Header offset.
  Elf32_Off		e_shoff;		// Section Header offset.
  Elf32_Word		e_flags;		// Processor specific flags.
  Elf32_Half		e_ehsize;		// ELF Header size.
  Elf32_Half		e_phentsize;		// Program Header Entry size.
  Elf32_Half		e_phnum;		// Number of entries in the Program Header.
  Elf32_Half		e_shentsize;		// Section Header Entry size.
  Elf32_Half		e_shnum;		// Number of entries in the Section Header.
  Elf32_Half		e_shstrndx;		// Section Header Index for the Section Header String Table.

  friend istream& operator>>(istream& is, Elf32_Ehdr& header) { is.read(&header, sizeof(Elf32_Ehdr)); return is; }
  inline bool check_format(void) const;
};

// Figure 1-4: e_ident[] identification indexes.
int const EI_MAG0 = 0;			// File identification.
int const EI_MAG1 = 1;			// File identification.
int const EI_MAG2 = 2;			// File identification.
int const EI_MAG3 = 3;			// File identification.
int const EI_CLASS = 4;			// File class.
int const EI_DATA = 5;			// Data encoding.
int const EI_VERSION = 6;		// File version.
int const EI_PAD = 7;			// Start of padding bytes.

// Possible values of e_ident[EI_CLASS].
unsigned char const ELFCLASSNONE = 0;	// Invalid class.
unsigned char const ELFCLASS32 = 1;	// 32-bit objects.
unsigned char const ELFCLASS64 = 2;	// 64-bit objects.

// Possible values of e_ident[EI_DATA].
unsigned char const ELFDATANONE = 0;	// Invalid data encoding.
unsigned char const ELFDATA2LSB = 1;	// Little endian encoding.
unsigned char const ELFDATAMSB = 2;	// Big endian encoding.

// Object file types (e_type).
Elf32_Half const ET_NONE = 0;		// No file type.
Elf32_Half const ET_REL = 1;		// Relocatable file.
Elf32_Half const ET_EXEC = 2;		// Executable file.
Elf32_Half const ET_DYN = 3;		// Shared object file.
Elf32_Half const ET_CORE = 4;		// Core file.

// Architecture types (e_machine).
Elf32_Half const EM_NONE = 0;		// No machine.
Elf32_Half const EM_M32 = 1;		// AT&T WE 32100
Elf32_Half const EM_SPARC = 2;		// SPARC
Elf32_Half const EM_386 = 3;		// Intel 80386
Elf32_Half const EM_68K = 4;		// Motorola 68000
Elf32_Half const EM_88K = 5;		// Motorola 80000
Elf32_Half const EM_860 = 7;		// Intel 80860
Elf32_Half const EM_MIPS = 8;		// MPIS RS3000

// ELF header version (e_version).
Elf32_Word const EV_NONE = 0;		// Invalid version.
Elf32_Word const EV_CURRENT = 1;	// Current version.

bool Elf32_Ehdr::check_format(void) const
{
  if (e_ident[EI_MAG0] != 0x7f ||
      e_ident[EI_MAG1] != 'E' ||
      e_ident[EI_MAG2] != 'L' ||
      e_ident[EI_MAG3] != 'F')
    Dout(dc::bfd, "Object file must be ELF.");
  else if (e_ident[EI_CLASS] != ELFCLASS32)
    Dout(dc::bfd, "Sorry, object file must be ELF32.");
  else if (e_ident[EI_DATA] !=
#if __BYTE_ORDER == __LITTLE_ENDIAN
      ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
      ELFDATAMSB
#else
      ELFDATANONE
#endif
      )
    Dout(dc::bfd, "Object file has non-native data encoding.");
  else if (e_ident[EI_VERSION] != EV_CURRENT)
    Dout(dc::warning, "Object file has different version than what libcwd understands.");
  else
    return false; 
  return true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------
// Section header table

// Figure 1-8: Special section indexes.
int const SHN_UNDEF = 0;		// Undefined, missing, irrelevant or otherwise meaningless section reference.
int const SHN_LORESERVE = 0xff00;	// This value specifies the lower bound of the range of reserved indexes.
int const SHN_LOPROC = 0xff00;		// Start of range reserved for processor-specific semnatics.
int const SHN_HIPROC = 0xff1f;		// End of range reserved for processor-specific semantics.
int const SHN_ABS = 0xfff1;		// This value specifies absolute values for the corresponding reference.
int const SHN_COMMON = 0xfff2;		// Symbols defined relative to this section are common symbols (such as unallocated C exteral variables).
int const SHN_HIRESERVE = 0xffff;	// This value specifies the upper bound of the range of reserved indexes.

struct Elf32_Shdr {
  Elf32_Word sh_name;			// Index into the section header string table ssection.  Specifies the name of the section.
  Elf32_Word sh_type;			// This member categorizes the sections contents and semantics.
  Elf32_Word sh_flags;			// 1-bit flags that describe miscellaneous attributes.
  Elf32_Addr sh_addr;			// Start address of section in memory image (or 0 otherwise).
  Elf32_Off sh_offset;			// Offset to first byte of section in the object file.
  Elf32_Word sh_size;			// Size of the section (unless the section is of type SHT_NOBITS).
  Elf32_Word sh_link;			// Section header table index link.
  Elf32_Word sh_info;			// Extra information.
  Elf32_Word sh_addralign;		// Alignment constraint, if any.
  Elf32_Word sh_entsize;		// Size of fixed-size entries in the section (or 0 otherwise).
};

//-------------------------------------------------------------------------------------------------------------------------------------------

class bfd_ct : public bfd_st {
private:
  char const* M_filename;
  ifstream M_input_stream;
  Elf32_Ehdr M_header;
  Elf32_Shdr* M_section_headers;
public:
  bfd_ct(char const* filename);
  ~bfd_ct() { if (M_section_headers) delete [] M_section_headers; }
protected:
  virtual bool check_format(void) const { return M_header.check_format(); }
  virtual long get_symtab_upper_bound(void);
  virtual long canonicalize_symtab(asymbol_st**);
  virtual void find_nearest_line(asection_st*, asymbol_st**, Elf32_Addr, char const**, char const**, unsigned int*) const;
};

bfd_st* bfd_st::openr(char const* filename)
{
  return new bfd_ct(filename);
}

long bfd_ct::get_symtab_upper_bound(void)
{
  ASSERT(M_header.e_shentsize == sizeof(Elf32_Shdr));
  if (M_header.e_shoff == 0 || M_header.e_shnum == 0)
    return 0;
  M_input_stream.rdbuf()->pubseekpos(M_header.e_shoff);
  M_section_headers = new Elf32_Shdr [M_header.e_shnum];
  M_input_stream.read(M_section_headers, M_header.e_shnum * sizeof(Elf32_Shdr));
  return 0;
}

long bfd_ct::canonicalize_symtab(asymbol_st**)
{
  Dout(dc::core, "bfd_ct::canonicalize_symtab: Unimplemented function");
  return 0;
}

void bfd_ct::find_nearest_line(asection_st*, asymbol_st**, Elf32_Addr, char const**, char const**, unsigned int*) const
{
  Dout(dc::core, "bfd_ct::find_nearest_line: Unimplemented function");
}

bfd_ct::bfd_ct(char const* filename) : M_filename(filename), M_section_headers(NULL)
{
  M_input_stream.open(filename);
  if (!M_input_stream)
    Dout(dc::fatal|error_cf, "fstream.open(\"" << filename << "\")");
  M_input_stream >> M_header;
}

} // namespace elf32

  } // namespace debug
} // namespace libcw
