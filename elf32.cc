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
// This file contains code that reads the symbol table and debug information from
// ELF32 object files.
//

#include <libcw/debug_config.h>

#ifndef DEBUGUSEGNULIBBFD

#include <libcw/sys.h>
#include <inttypes.h>	// ISO C99 header, needed for int32_t etc.
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif
#include <fstream>
#include <set>
#include <map>
#include <libcw/debug.h>
#include <libcw/elf32.h>

RCSTAG_CC("$Id$")

#undef DEBUGELF32
#undef DEBUGSTABS

namespace libcw {
  namespace debug {

namespace elf32 {

//==========================================================================================================================================
// The information about ELF (Executable Linkable Format) needed to write this file
// has been obtained from a file called 'ELF.doc.tar.gz'.  You can get this file from
// the net, for example: ftp://ftp.metalab.unc.edu/pub/Linux/GCC/ELF.doc.tar.gz.

//------------------------------------------------------------------------------------------------------------------------------------------
// ELF header table

// Figure 1-3: ELF Header.
static int const EI_NIDENT = 16;
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

  friend std::istream& operator>>(std::istream& is, Elf32_Ehdr& header) { is.read(reinterpret_cast<char*>(&header), sizeof(Elf32_Ehdr)); return is; }
  inline bool check_format(void) const;
};

// Figure 1-4: e_ident[] identification indexes.
static int const EI_MAG0 = 0;			// File identification.
static int const EI_MAG1 = 1;			// File identification.
static int const EI_MAG2 = 2;			// File identification.
static int const EI_MAG3 = 3;			// File identification.
static int const EI_CLASS = 4;			// File class.
static int const EI_DATA = 5;			// Data encoding.
static int const EI_VERSION = 6;		// File version.
static int const EI_PAD = 7;			// Start of padding bytes.

// Possible values of e_ident[EI_CLASS].
static unsigned char const ELFCLASSNONE = 0;	// Invalid class.
static unsigned char const ELFCLASS32 = 1;	// 32-bit objects.
static unsigned char const ELFCLASS64 = 2;	// 64-bit objects.

// Possible values of e_ident[EI_DATA].
static unsigned char const ELFDATANONE = 0;	// Invalid data encoding.
static unsigned char const ELFDATA2LSB = 1;	// Little endian encoding.
static unsigned char const ELFDATAMSB = 2;	// Big endian encoding.

// Object file types (e_type).
static Elf32_Half const ET_NONE = 0;		// No file type.
static Elf32_Half const ET_REL = 1;		// Relocatable file.
static Elf32_Half const ET_EXEC = 2;		// Executable file.
static Elf32_Half const ET_DYN = 3;		// Shared object file.
static Elf32_Half const ET_CORE = 4;		// Core file.

// Architecture types (e_machine).
static Elf32_Half const EM_NONE = 0;		// No machine.
static Elf32_Half const EM_M32 = 1;		// AT&T WE 32100
static Elf32_Half const EM_SPARC = 2;		// SPARC
static Elf32_Half const EM_386 = 3;		// Intel 80386
static Elf32_Half const EM_68K = 4;		// Motorola 68000
static Elf32_Half const EM_88K = 5;		// Motorola 80000
static Elf32_Half const EM_860 = 7;		// Intel 80860
static Elf32_Half const EM_MIPS = 8;		// MPIS RS3000

// ELF header version (e_version).
static Elf32_Word const EV_NONE = 0;		// Invalid version.
static Elf32_Word const EV_CURRENT = 1;	// Current version.

//-------------------------------------------------------------------------------------------------------------------------------------------
// Section header table

// Figure 1-8: Special section indexes.
static int const SHN_UNDEF = 0;		// Undefined, missing, irrelevant or otherwise meaningless section reference.
static int const SHN_LORESERVE = 0xff00;	// This value specifies the lower bound of the range of reserved indexes.
static int const SHN_LOPROC = 0xff00;		// Start of range reserved for processor-specific semnatics.
static int const SHN_HIPROC = 0xff1f;		// End of range reserved for processor-specific semantics.
static int const SHN_ABS = 0xfff1;		// This value specifies absolute values for the corresponding reference.
static int const SHN_COMMON = 0xfff2;		// Symbols defined relative to this section are common symbols (such as unallocated C exteral variables).
static int const SHN_HIRESERVE = 0xffff;	// This value specifies the upper bound of the range of reserved indexes.

// Figure 1-9: Section Header.
struct Elf32_Shdr {
  Elf32_Word sh_name;				// Index into the section header string table section.  Specifies the name of the section.
  Elf32_Word sh_type;				// This member categorizes the sections contents and semantics.
  Elf32_Word sh_flags;				// 1-bit flags that describe miscellaneous attributes.
  Elf32_Addr sh_addr;				// Start address of section in memory image (or 0 otherwise).
  Elf32_Off sh_offset;				// Offset to first byte of section in the object file.
  Elf32_Word sh_size;				// Size of the section (unless the section is of type SHT_NOBITS).
  Elf32_Word sh_link;				// Section header table index link.
  Elf32_Word sh_info;				// Extra information.
  Elf32_Word sh_addralign;			// Alignment constraint, if any.
  Elf32_Word sh_entsize;			// Size of fixed-size entries in the section (or 0 otherwise).
};

// Figure 1-10: Section Types, sh_type.
static Elf32_Word const SHT_NULL = 0;		// Inactive section.
static Elf32_Word const SHT_PROBITS = 1;	// Application specific section.
static Elf32_Word const SHT_SYMTAB = 2;		// Symbol table.
static Elf32_Word const SHT_STRTAB = 3;		// String table.
static Elf32_Word const SHT_RELA = 4;		// Relocation section with explicit addends.
static Elf32_Word const SHT_HASH = 5;		// Symbol hash table.
static Elf32_Word const SHT_DYNAMIC = 6;	// Dynamic section.
static Elf32_Word const SHT_NOTE = 7;		// Note section.
static Elf32_Word const SHT_NOBITS = 8;		// Empty section used as offset reference.
static Elf32_Word const SHT_REL = 9;		// Relocation section without explicit addends.
static Elf32_Word const SHT_SHLIB = 10;		// Reserved.
static Elf32_Word const SHT_DYNSYM = 11;	// Symbol table with minimal ammount of data required for dynamic linking.
static Elf32_Word const SHT_LOPROC = 0x70000000;// Start of range reserved for processor specific semantics.
static Elf32_Word const SHT_HIPROC = 0x7fffffff;// End of range reserved for processor specific semantics.
static Elf32_Word const SHT_LOUSER = 0x80000000;// Start of range reserved for user defined semantics.
static Elf32_Word const SHT_HIUSER = 0xffffffff;// End of range reserved for user defined semantics.

//-------------------------------------------------------------------------------------------------------------------------------------------
// Symbol Table

// Figure 1-16: Symbol Table Entry.
struct Elf32_sym {
  Elf32_Word st_name;				// Index into the symbol string table section.  Specifies the name of the symbol.
  Elf32_Addr st_value;				// The value of the associated symbol (absolute value, address, etc).
  Elf32_Word st_size;				// Size associated with this symbol, if any.
  unsigned char st_info;			// The symbols type and binding attributes.
  unsigned char st_other;			// Reserved.
  Elf32_Half st_shndx;				// Section header index relative to which this symbol is "defined".

  unsigned char bind(void) const { return st_info >> 4; }
  unsigned char type(void) const { return st_info & 0xf; }
};

// Figure 1-17: Symbol binding.
static int const STB_LOCAL = 0;			// Local symbols.
static int const STB_GLOBAL = 1;		// Global symbols.
static int const STB_WEAK = 2;			// Global symbols with weak linkage.
static int const STB_LOPROC = 13;		// Start of range reserved for processor-specific semantics.
static int const STB_HIPROC = 15;		// End of range reserved for processor-specific semantics.

// Figure 1-18: Symbol Types.
static int const STT_NOTYPE = 0;		// Unspecified type.
static int const STT_OBJECT = 1;		// Symbol is associated with a data object, such as a variable, an array, etc.
static int const STT_FUNC = 2;			// The symbol is associated with a function or other executable code.
static int const STT_SECTION = 3;		// The symbol is associated with a section (primarily used for relocation entries).
static int const STT_FILE = 4;			// The filename of the source file associated with the object file.
static int const STT_LOPROC = 13;		// Start of range reserved for processor-specific semantics.
static int const STT_HIPROC = 15;		// End of range reserved for processor-specific semantics.

//==========================================================================================================================================
// The information about stabs (Symbol TABleS) was obtained from http://www.informatik.uni-frankfurt.de/doc/texi/stabs_1.html.
//

// http://www.informatik.uni-frankfurt.de/doc/texi/stabs_6.html#SEC46
struct stab_st {
  Elf32_Word n_strx;         			// Index into string table of name.
  unsigned char n_type;         		// Type of symbol.
  unsigned char n_other;        		// Misc info (usually empty).
  Elf32_Half n_desc; 		       		// Description field.
  Elf32_Addr n_value;              		// Value of symbol.
};

// Type of symbol, n_type.
static unsigned char const N_GSYM = 0x20;
static unsigned char const N_FNAME = 0x22;
static unsigned char const N_FUN = 0x24;
static unsigned char const N_STSYM = 0x26;
static unsigned char const N_LCSYM = 0x28;
static unsigned char const N_MAIN = 0x2a;
static unsigned char const N_PC = 0x30;
static unsigned char const N_NSYMS = 0x32;
static unsigned char const N_NOMAP = 0x34;
static unsigned char const N_OBJ = 0x38;
static unsigned char const N_OPT = 0x3c;
static unsigned char const N_RSYM = 0x40;
static unsigned char const N_M2C = 0x42;
static unsigned char const N_SLINE = 0x44;
static unsigned char const N_DSLINE = 0x46;
static unsigned char const N_BSLINE = 0x48;
static unsigned char const N_BROWS = 0x48;
static unsigned char const N_DEFD = 0x4a;
static unsigned char const N_EHDECL = 0x50;
static unsigned char const N_MOD2 = 0x50;
static unsigned char const N_CATCH = 0x54;
static unsigned char const N_SSYM = 0x60;
static unsigned char const N_SO = 0x64;
static unsigned char const N_LSYM = 0x80;
static unsigned char const N_BINCL = 0x82;
static unsigned char const N_SOL = 0x84;
static unsigned char const N_PSYM = 0xa0;
static unsigned char const N_EINCL = 0xa2;
static unsigned char const N_ENTRY = 0xa4;
static unsigned char const N_LBRAC = 0xc0;
static unsigned char const N_EXCL = 0xc2;
static unsigned char const N_SCOPE = 0xc4;
static unsigned char const N_RBRAC = 0xe0;
static unsigned char const N_BCOMM = 0xe2;
static unsigned char const N_ECOMM = 0xe4;
static unsigned char const N_ECOML = 0xe8;
static unsigned char const N_NBTEXT = 0xF0;
static unsigned char const N_NBDATA = 0xF2;
static unsigned char const N_NBBSS = 0xF4;
static unsigned char const N_NBSTS = 0xF6;
static unsigned char const N_NBLCS = 0xF8;
static unsigned char const N_LENG = 0xfe;

//==========================================================================================================================================
// struct location_st
//
// Internal representation for locations.
//

struct location_st {
  std::set<std::string>::iterator source_iter;
  Elf32_Half line;
  std::set<std::string>::iterator func_iter;
};

struct range_st {
  Elf32_Addr start;
  size_t size;
};

bool operator==(range_st const& range1, range_st const& range2)
{
  DoutFatal(dc::core, "Calling operator==(range_st const& range1, range_st const& range2)");
}

#ifdef DEBUGSTABS
std::ostream& operator<<(std::ostream& os, range_st const& range)
{
  os << std::hex << range.start << " - " << range.start + range.size << ".";
  return os;
}

std::ostream& operator<<(std::ostream& os, std::pair<range_st const, location_st> const& p)
{
  os << std::hex << p.first.start << " - " << p.first.start + p.first.size
      << "; " << (*p.second.source_iter).data() << ':' << std::dec << p.second.line << " : \"" << (*p.second.func_iter).data() << "\".";
  return os;
}
#endif

struct compare_range_st {
  bool operator()(range_st const& range1, range_st const& range2) const { return range1.start >= range2.start + range2.size; }
};

//==========================================================================================================================================
// class section_ct
//
// This object represents an ELF section header.
//

class section_ct : public asection_st {
private:
  Elf32_Shdr M_section_header;
public:
  section_ct(void) { }
  void init(char const* section_header_string_table, Elf32_Shdr const& section_header);
  Elf32_Shdr const& section_header(void) const { return M_section_header; }
};

//
// class object_file_ct
//
// This object represents an ELF object file.
//

class object_file_ct : public bfd_st {
private:
  std::ifstream M_input_stream;
  Elf32_Ehdr M_header;
  char* M_section_header_string_table;
  section_ct* M_sections;
  char* M_symbol_string_table;
  char* M_dyn_symbol_string_table;
  asymbol_st* M_symbols;
  int M_number_of_symbols;
  Elf32_Word M_symbol_table_type;
  std::set<std::string> M_function_names;
  std::set<std::string> M_source_files;
  std::map<range_st, location_st, compare_range_st> M_ranges;
  Elf32_Word M_stabs_section_index;
  stab_st* M_stabs;
public:
  object_file_ct(char const* file_name);
  ~object_file_ct()
  {
    delete [] M_section_header_string_table;
    delete [] M_sections;
    delete [] M_symbol_string_table;
    delete [] M_dyn_symbol_string_table;
    delete [] M_symbols;
  }
  char const* get_section_header_string_table(void) const { return M_section_header_string_table; }
  section_ct const& get_section(int index) const { ASSERT( index < M_header.e_shnum ); return M_sections[index]; }
protected:
  virtual bool check_format(void) const { return M_header.check_format(); }
  virtual long get_symtab_upper_bound(void);
  virtual long canonicalize_symtab(asymbol_st**);
  virtual void find_nearest_line(asymbol_st const*, Elf32_Addr, char const**, char const**, unsigned int*);
private:
  char* allocate_and_read_section(int i);
  void register_range(location_st const& location, range_st const& range);
  void load_stabs(void);
};

//-------------------------------------------------------------------------------------------------------------------------------------------
// Implementation

bool Elf32_Ehdr::check_format(void) const
{
  if (e_ident[EI_MAG0] != 0x7f ||
      e_ident[EI_MAG1] != 'E' ||
      e_ident[EI_MAG2] != 'L' ||
      e_ident[EI_MAG3] != 'F')
    Dout(dc::stabs, "Object file must be ELF.");
  else if (e_ident[EI_CLASS] != ELFCLASS32)
    Dout(dc::stabs, "Sorry, object file must be ELF32.");
  else if (e_ident[EI_DATA] !=
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __LITTLE_ENDIAN
      ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
      ELFDATAMSB
#else
      ELFDATANONE
#endif
#else // !__BYTE_ORDER
#ifdef WORDS_BIGENDIAN
      ELFDATAMSB
#else
      ELFDATA2LSB
#endif
#endif // !__BYTE_ORDER
      )
    Dout(dc::stabs, "Object file has non-native data encoding.");
  else if (e_ident[EI_VERSION] != EV_CURRENT)
    Dout(dc::warning, "Object file has different version than what libcwd understands.");
  else
    return false; 
  return true;
}

void section_ct::init(char const* section_header_string_table, Elf32_Shdr const& section_header)
{
  std::memcpy(&M_section_header, &section_header, sizeof(M_section_header));
  // Duplicated values:
  vma = M_section_header.sh_addr;
  name = &section_header_string_table[M_section_header.sh_name];
}

bfd_st* bfd_st::openr(char const* file_name)
{
  return new object_file_ct(file_name);
}

long object_file_ct::get_symtab_upper_bound(void)
{
  return M_number_of_symbols * sizeof(asymbol_st*);
}

long object_file_ct::canonicalize_symtab(asymbol_st** symbol_table)
{
  M_symbols = new asymbol_st[M_number_of_symbols];
  asymbol_st* new_symbol = M_symbols;
  int table_entries = 0;
  for(int i = 0; i < M_header.e_shnum; ++i)
  {
    if ((M_sections[i].section_header().sh_type == M_symbol_table_type)
        && M_sections[i].section_header().sh_size > 0)
    {
      int number_of_symbols = M_sections[i].section_header().sh_size / sizeof(Elf32_sym);
#ifdef DEBUGELF32
      Dout(dc::stabs, "Found symbol table " << M_sections[i].name << " with " << number_of_symbols << " symbols.");
#endif
      Elf32_sym* symbols = (Elf32_sym*)allocate_and_read_section(i);
      for(int s = 0; s < number_of_symbols; ++s)
      {
	Elf32_sym& symbol(symbols[s]);
        if (symbol.st_shndx >= SHN_LORESERVE || symbol.st_shndx == SHN_UNDEF)
	  continue;							// Skip Special Sections and Undefined Symbols.
	new_symbol->bfd_ptr = this;
	new_symbol->section = &M_sections[symbol.st_shndx];
	if (M_sections[i].section_header().sh_type == SHT_SYMTAB)
	  new_symbol->name = &M_symbol_string_table[symbol.st_name];
	else
	  new_symbol->name = &M_dyn_symbol_string_table[symbol.st_name];
	if (!*new_symbol->name)
	  continue;							// Skip Symbols that do not have a name.
#ifdef DEBUGELF32
	Dout(dc::stabs, "Symbol \"" << new_symbol->name << "\" in section \"" << new_symbol->section->name << "\".");
#endif
	new_symbol->value = symbol.st_value - new_symbol->section->vma;	// Is not an absolute value: make value relative to start of section.
	new_symbol->udata.p = symbol.st_size;
	new_symbol->flags = 0;
	switch(symbol.bind())
	{
	  case STB_LOCAL:
	    new_symbol->flags |= cwbfd::BSF_LOCAL;
	    break;
	  case STB_GLOBAL:
	    new_symbol->flags |= cwbfd::BSF_GLOBAL;
	    break;
	  case STB_WEAK:
	    new_symbol->flags |= cwbfd::BSF_WEAK;
            break;
	  default:	// Ignored
	    break;
	}
        switch(symbol.type())
	{
	  case STT_OBJECT:
	    new_symbol->flags |= cwbfd::BSF_OBJECT;
	    break;
	  case STT_FUNC:
	    new_symbol->flags |= cwbfd::BSF_FUNCTION;
	    break;
	  case STT_SECTION:
	    new_symbol->flags |= cwbfd::BSF_SECTION_SYM;
	    break;
	  case STT_FILE:
	    new_symbol->flags |= cwbfd::BSF_FILE;
	    break;
	}
	symbol_table[table_entries++] = new_symbol++;
      }
      delete [] symbols;
      break;							// There *should* only be one symbol table section.
    }
  }
  ASSERT( M_number_of_symbols >= table_entries );
  M_number_of_symbols = table_entries;
  return M_number_of_symbols;
}

void object_file_ct::load_stabs(void)
{
  if (M_stabs_section_index == 0)
    return;
#ifdef DEBUGSTABS
  ASSERT( M_sections[i].section_header().sh_entsize == sizeof(stab_st) );
#endif
  stab_st* stabs = (stab_st*)allocate_and_read_section(M_stabs_section_index);
#ifdef DEBUGSTABS
  ASSERT( !strcmp(&M_section_header_string_table[M_sections[M_sections[i].section_header().sh_link].section_header().sh_name], ".stabstr") );
  ASSERT( stabs->n_desc == (Elf32_Half)(M_sections[i].section_header().sh_size / M_sections[i].section_header().sh_entsize - 1) );
#endif
  char* stabs_string_table = allocate_and_read_section(M_sections[M_stabs_section_index].section_header().sh_link);
#ifdef DEBUGSTABS
  Debug( libcw_do.inc_indent(4) );
#endif
  Elf32_Addr func_addr;
  std::string cur_dir;
  std::string cur_source;
  std::string cur_func;
  location_st location;
  range_st range;
  for (unsigned int j = 0; j < M_sections[M_stabs_section_index].section_header().sh_size / M_sections[M_stabs_section_index].section_header().sh_entsize; ++j)
  {
    switch(stabs[j].n_type)
    {
      case N_SO:
      case N_SOL:
      {
	char const* filename = &stabs_string_table[stabs[j].n_strx];
	if (*filename == '/')
	{
	  cur_source.assign(filename);
	  cur_source += '\0';
	  if (stabs[j].n_type == N_SO)
	  {
	    cur_dir.assign(filename);
	    ASSERT( *(cur_dir.rbegin()) == '/' );
	  }
	  else
	    location.source_iter = M_source_files.insert(cur_source).first; 
	    location.line = 0;	// See N_SLINE
	}
	else
	{
	  cur_source = cur_dir;
	  cur_source += filename;
	  cur_source += '\0';
	  location.source_iter = M_source_files.insert(cur_source).first; 
	  location.line = 0;
	}
#ifdef DEBUGSTABS
	Dout(dc::stabs, ((stabs[j].n_type  == N_SO) ? "N_SO : \"" : "N_SOL: \"") << cur_source.data() << "\".");
#endif
	break;
      }
      case N_FUN:
      {
	if (stabs[j].n_strx == 0)
	{
#ifdef DEBUGSTABS
	  Dout(dc::stabs, "N_FUN: " << "end at " << std::hex << stabs[j].n_value << '.');
#endif
	  range.size = func_addr + stabs[j].n_value - range.start;
	  register_range(location, range);
	}
	else
	{
	  range.start = func_addr = stabs[j].n_value;
	  char const* fn = &stabs_string_table[stabs[j].n_strx];
	  cur_func.assign(fn, strchr(fn, ':') - fn);
	  cur_func += '\0';
	  location.func_iter = M_function_names.insert(cur_func).first;
	  location.line = 0;
#ifdef DEBUGSTABS
	  Dout(dc::stabs, "N_FUN: " << std::hex << func_addr << " : \"" << &stabs_string_table[stabs[j].n_strx] << "\".");
#endif
	}
	break;
      }
      case N_SLINE:
#ifdef DEBUGSTABS
	Dout(dc::stabs, "N_SLINE: " << stabs[j].n_desc << " at " << std::hex << stabs[j].n_value << '.');
#endif
	if (stabs[j].n_value != 0)
	{
	  // Always false when source or function was changed since last line because location.line is set to 0 in that case.
	  if (location.line == stabs[j].n_desc)	// Catenate ranges with same location.
	    break;
	  range.size = func_addr + stabs[j].n_value - range.start;
	  register_range(location, range);
	  range.start += range.size;
	}
	location.line = stabs[j].n_desc;
	break;
    }
  }
#ifdef DEBUGSTABS
  Debug( libcw_do.dec_indent(4) );
#endif
  delete [] stabs;
  delete [] stabs_string_table;
  M_stabs_section_index = 0;
}

void object_file_ct::find_nearest_line(asymbol_st const* symbol, Elf32_Addr offset, char const** file, char const** func, unsigned int* line)
{
  load_stabs();
  range_st range;
  range.start = offset;
  range.size = 1;
  std::map<range_st, location_st, compare_range_st>::const_iterator i(M_ranges.find(static_cast<range_st const>(range)));
  if (i == M_ranges.end())
  {
    *file = NULL;
    *func = symbol->name;
    *line = 0;
  }
  else
  {
    *file = (*(*i).second.source_iter).data();
    *func = (*(*i).second.func_iter).data();
    *line = (*i).second.line;
  }
  return;
}

char* object_file_ct::allocate_and_read_section(int i)
{
  char* p = new char[M_sections[i].section_header().sh_size];
  M_input_stream.rdbuf()->pubseekpos(M_sections[i].section_header().sh_offset);
  M_input_stream.read(p, M_sections[i].section_header().sh_size);
  return p;
}

void object_file_ct::register_range(location_st const& location, range_st const& range)
{
#ifdef DEBUGSTABS
  Dout(dc::stabs, std::hex << range.start << " - " << (range.start + range.size)
      << "; " << (*location.source_iter).data() << ':' << std::dec << location.line << " : \""
      << (*location.func_iter).data() << "\".");
  std::pair<std::map<range_st, location_st, compare_range_st>::iterator, bool> p(
#endif
      M_ranges.insert(std::pair<range_st, location_st>(range, location))
#ifdef DEBUGSTABS
      )
#endif
      ;
#ifdef DEBUGSTABS
  if (!p.second)
  {
    if ((*p.first).second.func_iter != location.func_iter)
      Dout(dc::stabs, "WARNING: Collision between different functions (" << *p.first << ")!?");
    else
    {
      if ((*p.first).first.start != range.start )
        Dout(dc::stabs, "WARNING: Different start for same function (" << *p.first << ")!?");
      if ((*p.first).first.size != range.size)
	Dout(dc::stabs, "WARNING: Different sizes for same function.  Not sure what .stabs entry to use.");
      if ((*p.first).second.line != location.line)
        Dout(dc::stabs, "WARNING: Different line numbers for overlapping range (" << *p.first << ")!?");
      if ((*p.first).second.source_iter != location.source_iter)
        Dout(dc::stabs, "Collision with " << *p.first << ".");
    }
  }
#endif
}

object_file_ct::object_file_ct(char const* file_name) :
    M_section_header_string_table(NULL), M_sections(NULL), M_symbol_string_table(NULL),  M_dyn_symbol_string_table(NULL),
    M_symbols(NULL), M_number_of_symbols(0), M_symbol_table_type(0)
{
  filename = file_name;
  M_input_stream.open(file_name);
  if (!M_input_stream)
    Dout(dc::fatal|error_cf, "std::fstream.open(\"" << file_name << "\")");
  M_input_stream >> M_header;
  ASSERT(M_header.e_shentsize == sizeof(Elf32_Shdr));
  if (M_header.e_shoff == 0 || M_header.e_shnum == 0)
    return;
  M_input_stream.rdbuf()->pubseekpos(M_header.e_shoff);
  Elf32_Shdr* section_headers = new Elf32_Shdr [M_header.e_shnum];
  M_input_stream.read(reinterpret_cast<char*>(section_headers), M_header.e_shnum * sizeof(Elf32_Shdr));
#ifdef DEBUGELF32
  Dout(dc::stabs, "Number of section headers: " << M_header.e_shnum);
#endif
  ASSERT( section_headers[M_header.e_shstrndx].sh_size > 0
      && section_headers[M_header.e_shstrndx].sh_size >= section_headers[M_header.e_shstrndx].sh_name );
  M_section_header_string_table = new char[section_headers[M_header.e_shstrndx].sh_size]; 
  M_input_stream.rdbuf()->pubseekpos(section_headers[M_header.e_shstrndx].sh_offset);
  M_input_stream.read(M_section_header_string_table, section_headers[M_header.e_shstrndx].sh_size);
  ASSERT( !strcmp(&M_section_header_string_table[section_headers[M_header.e_shstrndx].sh_name], ".shstrtab") );
  M_sections = new section_ct[M_header.e_shnum];
#ifdef DEBUGELF32
  Debug( libcw_do.inc_indent(4) );
#endif
  M_stabs_section_index = 0;
  for(int i = 0; i < M_header.e_shnum; ++i)
  {
#ifdef DEBUGELF32
    if (section_headers[i].sh_name)
      Dout(dc::stabs, "Section name: \"" << &M_section_header_string_table[section_headers[i].sh_name] << '"');
#endif
    M_sections[i].init(M_section_header_string_table, section_headers[i]);
    if (!strcmp(M_sections[i].name, ".strtab"))
      M_symbol_string_table = allocate_and_read_section(i);
    else if (!strcmp(M_sections[i].name, ".dynstr"))
      M_dyn_symbol_string_table = allocate_and_read_section(i);
    else if (!strcmp(M_sections[i].name, ".stab"))
      M_stabs_section_index = i;
    if ((section_headers[i].sh_type == SHT_SYMTAB || section_headers[i].sh_type == SHT_DYNSYM)
        && section_headers[i].sh_size > 0)
    {
      M_has_syms = true;
      ASSERT( section_headers[i].sh_entsize == sizeof(Elf32_sym) );
      ASSERT( M_symbol_table_type != SHT_SYMTAB || section_headers[i].sh_type != SHT_SYMTAB);	// There should only be one SHT_SYMTAB.
      if (M_symbol_table_type != SHT_SYMTAB)							// If there is one, use it.
      {
	M_symbol_table_type = section_headers[i].sh_type;
	M_number_of_symbols = section_headers[i].sh_size / section_headers[i].sh_entsize;
      }
    }
  }
#ifdef DEBUGELF32
  Debug( libcw_do.dec_indent(4) );
  Dout(dc::stabs, "Number of symbols: " << M_number_of_symbols);
#endif
  delete [] section_headers;
  // load_stabs();	// Preload stabs.
}

} // namespace elf32

  } // namespace debug
} // namespace libcw

#endif // !DEBUGUSEGNULIBBFD
