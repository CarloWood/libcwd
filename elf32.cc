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

#include "sys.h"
#include <inttypes.h>	// ISO C99 header, needed for int32_t etc.
#include <iomanip>
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include <libcw/debug.h>
#include <libcw/elf32.h>

RCSTAG_CC("$Id$")

#define DEBUGELF32 0
#define DEBUGSTABS 0
#define DEBUGDWARF 0

#if DEBUGDWARF
#define DoutDwarf(cntrl, x) Dout(cntrl, x)
#else
#define DoutDwarf(cntrl, x)
#endif

// This assumes that DW_TAG_compile_unit is the first tag for each compile unit
// which is not strictly garanteed in the standard but seems to be the case.
// Moreover it assumes that DW_TAG_compile_unit is the first entry in the
// .debug_info section for each compile unit.
#define DWARFSPEEDUP

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
// The information about DWARF was obtained from http://www.eagercon.com/dwarf/dwarf-2.0.0.pdf
// which is one of the worsed documentations I ever saw :/.
//

typedef long LEB128_t;
static int const number_of_bits_in_LEB128_t = 8 * sizeof(LEB128_t);
typedef unsigned long uLEB128_t;
static int const number_of_bits_in_uLEB128_t = 8 * sizeof(uLEB128_t);

static uLEB128_t const DW_TAG_array_type		= 0x01;
static uLEB128_t const DW_TAG_class_type		= 0x02;
static uLEB128_t const DW_TAG_entry_point		= 0x03;
static uLEB128_t const DW_TAG_enumeration_type		= 0x04;
static uLEB128_t const DW_TAG_formal_parameter		= 0x05;
static uLEB128_t const DW_TAG_imported_declaration	= 0x08;
static uLEB128_t const DW_TAG_label			= 0x0a;
static uLEB128_t const DW_TAG_lexical_block		= 0x0b;
static uLEB128_t const DW_TAG_member			= 0x0d;
static uLEB128_t const DW_TAG_pointer_type		= 0x0f;
static uLEB128_t const DW_TAG_reference_type		= 0x10;
static uLEB128_t const DW_TAG_compile_unit		= 0x11;
static uLEB128_t const DW_TAG_string_type		= 0x12;
static uLEB128_t const DW_TAG_structure_type		= 0x13;
static uLEB128_t const DW_TAG_subroutine_type		= 0x15;
static uLEB128_t const DW_TAG_typedef			= 0x16;
static uLEB128_t const DW_TAG_union_type		= 0x17;
static uLEB128_t const DW_TAG_unspecified_parameters	= 0x18;
static uLEB128_t const DW_TAG_variant			= 0x19;
static uLEB128_t const DW_TAG_common_block		= 0x1a;
static uLEB128_t const DW_TAG_common_inclusion		= 0x1b;
static uLEB128_t const DW_TAG_inheritance		= 0x1c;
static uLEB128_t const DW_TAG_inlined_subroutine	= 0x1d;
static uLEB128_t const DW_TAG_module			= 0x1e;
static uLEB128_t const DW_TAG_ptr_to_member_type	= 0x1f;
static uLEB128_t const DW_TAG_set_type			= 0x20;
static uLEB128_t const DW_TAG_subrange_type		= 0x21;
static uLEB128_t const DW_TAG_with_stmt			= 0x22;
static uLEB128_t const DW_TAG_access_declaration	= 0x23;
static uLEB128_t const DW_TAG_base_type			= 0x24;
static uLEB128_t const DW_TAG_catch_block		= 0x25;
static uLEB128_t const DW_TAG_const_type		= 0x26;
static uLEB128_t const DW_TAG_constant			= 0x27;
static uLEB128_t const DW_TAG_enumerator		= 0x28;
static uLEB128_t const DW_TAG_file_type			= 0x29;
static uLEB128_t const DW_TAG_friend			= 0x2a;
static uLEB128_t const DW_TAG_namelist			= 0x2b;
static uLEB128_t const DW_TAG_namelist_item		= 0x2c;
static uLEB128_t const DW_TAG_packed_type		= 0x2d;
static uLEB128_t const DW_TAG_subprogram		= 0x2e;
static uLEB128_t const DW_TAG_template_type_param	= 0x2f;
static uLEB128_t const DW_TAG_template_value_param	= 0x30;
static uLEB128_t const DW_TAG_thrown_type		= 0x31;
static uLEB128_t const DW_TAG_try_block			= 0x32;
static uLEB128_t const DW_TAG_variant_part		= 0x33;
static uLEB128_t const DW_TAG_variable			= 0x34;
static uLEB128_t const DW_TAG_volatile_type		= 0x35;
static uLEB128_t const DW_TAG_lo_user			= 0x4080;
static uLEB128_t const DW_TAG_hi_user			= 0xffff;

#if DEBUGDWARF
char const* print_DW_TAG_name(uLEB128_t tag)
{
  switch(tag)
  {
    case DW_TAG_array_type: return "DW_TAG_array_type";
    case DW_TAG_class_type: return "DW_TAG_class_type";
    case DW_TAG_entry_point: return "DW_TAG_entry_point";
    case DW_TAG_enumeration_type: return "DW_TAG_enumeration_type";
    case DW_TAG_formal_parameter: return "DW_TAG_formal_parameter";
    case DW_TAG_imported_declaration: return "DW_TAG_imported_declaration";
    case DW_TAG_label: return "DW_TAG_label";
    case DW_TAG_lexical_block: return "DW_TAG_lexical_block";
    case DW_TAG_member: return "DW_TAG_member";
    case DW_TAG_pointer_type: return "DW_TAG_pointer_type";
    case DW_TAG_reference_type: return "DW_TAG_reference_type";
    case DW_TAG_compile_unit: return "DW_TAG_compile_unit";
    case DW_TAG_string_type: return "DW_TAG_string_type";
    case DW_TAG_structure_type: return "DW_TAG_structure_type";
    case DW_TAG_subroutine_type: return "DW_TAG_subroutine_type";
    case DW_TAG_typedef: return "DW_TAG_typedef";
    case DW_TAG_union_type: return "DW_TAG_union_type";
    case DW_TAG_unspecified_parameters: return "DW_TAG_unspecified_parameters";
    case DW_TAG_variant: return "DW_TAG_variant";
    case DW_TAG_common_block: return "DW_TAG_common_block";
    case DW_TAG_common_inclusion: return "DW_TAG_common_inclusion";
    case DW_TAG_inheritance: return "DW_TAG_inheritance";
    case DW_TAG_inlined_subroutine: return "DW_TAG_inlined_subroutine";
    case DW_TAG_module: return "DW_TAG_module";
    case DW_TAG_ptr_to_member_type: return "DW_TAG_ptr_to_member_type";
    case DW_TAG_set_type: return "DW_TAG_set_type";
    case DW_TAG_subrange_type: return "DW_TAG_subrange_type";
    case DW_TAG_with_stmt: return "DW_TAG_with_stmt";
    case DW_TAG_access_declaration: return "DW_TAG_access_declaration";
    case DW_TAG_base_type: return "DW_TAG_base_type";
    case DW_TAG_catch_block: return "DW_TAG_catch_block";
    case DW_TAG_const_type: return "DW_TAG_const_type";
    case DW_TAG_constant: return "DW_TAG_constant";
    case DW_TAG_enumerator: return "DW_TAG_enumerator";
    case DW_TAG_file_type: return "DW_TAG_file_type";
    case DW_TAG_friend: return "DW_TAG_friend";
    case DW_TAG_namelist: return "DW_TAG_namelist";
    case DW_TAG_namelist_item: return "DW_TAG_namelist_item";
    case DW_TAG_packed_type: return "DW_TAG_packed_type";
    case DW_TAG_subprogram: return "DW_TAG_subprogram";
    case DW_TAG_template_type_param: return "DW_TAG_template_type_param";
    case DW_TAG_template_value_param: return "DW_TAG_template_value_param";
    case DW_TAG_thrown_type: return "DW_TAG_thrown_type";
    case DW_TAG_try_block: return "DW_TAG_try_block";
    case DW_TAG_variant_part: return "DW_TAG_variant_part";
    case DW_TAG_variable: return "DW_TAG_variable";
    case DW_TAG_volatile_type: return "DW_TAG_volatile_type";
    case DW_TAG_lo_user: return "DW_TAG_lo_user";
    case DW_TAG_hi_user: return "DW_TAG_hi_user";
  }
  return "UNKNOWN DW_TAG";
}
#endif

static unsigned char const DW_CHILDREN_no	= 0;
static unsigned char const DW_CHILDREN_yes	= 1;

static uLEB128_t const DW_AT_sibling 		= 0x01;	// reference
static uLEB128_t const DW_AT_location 		= 0x02;	// block, constant
static uLEB128_t const DW_AT_name 		= 0x03;	// string
static uLEB128_t const DW_AT_ordering 		= 0x09;	// constant
static uLEB128_t const DW_AT_byte_size 		= 0x0b;	// constant
static uLEB128_t const DW_AT_bit_offset 	= 0x0c;	// constant
static uLEB128_t const DW_AT_bit_size 		= 0x0d;	// constant
static uLEB128_t const DW_AT_stmt_list 		= 0x10;	// constant
static uLEB128_t const DW_AT_low_pc 		= 0x11;	// address
static uLEB128_t const DW_AT_high_pc 		= 0x12;	// address
static uLEB128_t const DW_AT_language 		= 0x13;	// constant
static uLEB128_t const DW_AT_discr 		= 0x15;	// reference
static uLEB128_t const DW_AT_discr_value 	= 0x16;	// constant
static uLEB128_t const DW_AT_visibility 	= 0x17;	// constant
static uLEB128_t const DW_AT_import 		= 0x18;	// reference
static uLEB128_t const DW_AT_string_length 	= 0x19;	// block, constant
static uLEB128_t const DW_AT_common_reference 	= 0x1a;	// reference
static uLEB128_t const DW_AT_comp_dir 		= 0x1b;	// string
static uLEB128_t const DW_AT_const_value 	= 0x1c;	// string, constant, block
static uLEB128_t const DW_AT_containing_type 	= 0x1d;	// reference
static uLEB128_t const DW_AT_default_value 	= 0x1e;	// reference
static uLEB128_t const DW_AT_inline 		= 0x20;	// constant
static uLEB128_t const DW_AT_is_optional 	= 0x21;	// flag
static uLEB128_t const DW_AT_lower_bound 	= 0x22;	// constant, reference
static uLEB128_t const DW_AT_producer 		= 0x25;	// string
static uLEB128_t const DW_AT_prototyped 	= 0x27;	// flag
static uLEB128_t const DW_AT_return_addr 	= 0x2a;	// block, constant
static uLEB128_t const DW_AT_start_scope 	= 0x2c;	// constant
static uLEB128_t const DW_AT_stride_size 	= 0x2e;	// constant
static uLEB128_t const DW_AT_upper_bound 	= 0x2f;	// constant, reference
static uLEB128_t const DW_AT_abstract_origin 	= 0x31;	// reference
static uLEB128_t const DW_AT_accessibility 	= 0x32;	// constant
static uLEB128_t const DW_AT_address_class 	= 0x33;	// constant
static uLEB128_t const DW_AT_artificial 	= 0x34;	// flag
static uLEB128_t const DW_AT_base_types 	= 0x35;	// reference
static uLEB128_t const DW_AT_calling_convention	= 0x36;	// constant
static uLEB128_t const DW_AT_count 		= 0x37;	// constant, reference
static uLEB128_t const DW_AT_data_member_location = 0x38; // block, reference
static uLEB128_t const DW_AT_decl_column 	= 0x39;	// constant
static uLEB128_t const DW_AT_decl_file 		= 0x3a;	// constant
static uLEB128_t const DW_AT_decl_line 		= 0x3b;	// constant
static uLEB128_t const DW_AT_declaration 	= 0x3c;	// flag
static uLEB128_t const DW_AT_discr_list 	= 0x3d;	// block
static uLEB128_t const DW_AT_encoding 		= 0x3e;	// constant
static uLEB128_t const DW_AT_external 		= 0x3f;	// flag
static uLEB128_t const DW_AT_frame_base 	= 0x40;	// block, constant
static uLEB128_t const DW_AT_friend 		= 0x41;	// reference
static uLEB128_t const DW_AT_identifier_case 	= 0x42;	// constant
static uLEB128_t const DW_AT_macro_info 	= 0x43;	// constant
static uLEB128_t const DW_AT_namelist_item 	= 0x44;	// block
static uLEB128_t const DW_AT_priority 		= 0x45;	// reference
static uLEB128_t const DW_AT_segment 		= 0x46;	// block, constant
static uLEB128_t const DW_AT_specification 	= 0x47;	// reference
static uLEB128_t const DW_AT_static_link 	= 0x48;	// block, constant
static uLEB128_t const DW_AT_type 		= 0x49;	// reference
static uLEB128_t const DW_AT_use_location 	= 0x4a;	// block, constant
static uLEB128_t const DW_AT_variable_parameter	= 0x4b;	// flag
static uLEB128_t const DW_AT_virtuality 	= 0x4c;	// constant
static uLEB128_t const DW_AT_vtable_elem_location = 0x4d; // block, reference
static uLEB128_t const DW_AT_lo_user		= 0x2000;
static uLEB128_t const DW_AT_hi_user		= 0x3fff;

#if DEBUGDWARF
char const* print_DW_AT_name(uLEB128_t attr)
{
  switch(attr)
  {
    case DW_AT_sibling: return "DW_AT_sibling";
    case DW_AT_location: return "DW_AT_location";
    case DW_AT_name: return "DW_AT_name";
    case DW_AT_ordering: return "DW_AT_ordering";
    case DW_AT_byte_size: return "DW_AT_byte_size";
    case DW_AT_bit_offset: return "DW_AT_bit_offset";
    case DW_AT_bit_size: return "DW_AT_bit_size";
    case DW_AT_stmt_list: return "DW_AT_stmt_list";
    case DW_AT_low_pc: return "DW_AT_low_pc";
    case DW_AT_high_pc: return "DW_AT_high_pc";
    case DW_AT_language: return "DW_AT_language";
    case DW_AT_discr: return "DW_AT_discr";
    case DW_AT_discr_value: return "DW_AT_discr_value";
    case DW_AT_visibility: return "DW_AT_visibility";
    case DW_AT_import: return "DW_AT_import";
    case DW_AT_string_length: return "DW_AT_string_length";
    case DW_AT_common_reference: return "DW_AT_common_reference";
    case DW_AT_comp_dir: return "DW_AT_comp_dir";
    case DW_AT_const_value: return "DW_AT_const_value";
    case DW_AT_containing_type: return "DW_AT_containing_type";
    case DW_AT_default_value: return "DW_AT_default_value";
    case DW_AT_inline: return "DW_AT_inline";
    case DW_AT_is_optional: return "DW_AT_is_optional";
    case DW_AT_lower_bound: return "DW_AT_lower_bound";
    case DW_AT_producer: return "DW_AT_producer";
    case DW_AT_prototyped: return "DW_AT_prototyped";
    case DW_AT_return_addr: return "DW_AT_return_addr";
    case DW_AT_start_scope: return "DW_AT_start_scope";
    case DW_AT_stride_size: return "DW_AT_stride_size";
    case DW_AT_upper_bound: return "DW_AT_upper_bound";
    case DW_AT_abstract_origin: return "DW_AT_abstract_origin";
    case DW_AT_accessibility: return "DW_AT_accessibility";
    case DW_AT_address_class: return "DW_AT_address_class";
    case DW_AT_artificial: return "DW_AT_artificial";
    case DW_AT_base_types: return "DW_AT_base_types";
    case DW_AT_calling_convention: return "DW_AT_calling_convention";
    case DW_AT_count: return "DW_AT_count";
    case DW_AT_data_member_location: return "DW_AT_data_member_location";
    case DW_AT_decl_column: return "DW_AT_decl_column";
    case DW_AT_decl_file: return "DW_AT_decl_file";
    case DW_AT_decl_line: return "DW_AT_decl_line";
    case DW_AT_declaration: return "DW_AT_declaration";
    case DW_AT_discr_list: return "DW_AT_discr_list";
    case DW_AT_encoding: return "DW_AT_encoding";
    case DW_AT_external: return "DW_AT_external";
    case DW_AT_frame_base: return "DW_AT_frame_base";
    case DW_AT_friend: return "DW_AT_friend";
    case DW_AT_identifier_case: return "DW_AT_identifier_case";
    case DW_AT_macro_info: return "DW_AT_macro_info";
    case DW_AT_namelist_item: return "DW_AT_namelist_item";
    case DW_AT_priority: return "DW_AT_priority";
    case DW_AT_segment: return "DW_AT_segment";
    case DW_AT_specification: return "DW_AT_specification";
    case DW_AT_static_link: return "DW_AT_static_link";
    case DW_AT_type: return "DW_AT_type";
    case DW_AT_use_location: return "DW_AT_use_location";
    case DW_AT_variable_parameter: return "DW_AT_variable_parameter";
    case DW_AT_virtuality: return "DW_AT_virtuality";
    case DW_AT_vtable_elem_location: return "DW_AT_vtable_elem_location";
    case DW_AT_lo_user: return "DW_AT_lo_user";
    case DW_AT_hi_user: return "DW_AT_hi_user";
    case 0: return "0";
  }
  return "UNKNOWN DW_AT";
}
#endif

static uLEB128_t const DW_FORM_addr		= 0x01; // address
static uLEB128_t const DW_FORM_block2		= 0x03; // block
static uLEB128_t const DW_FORM_block4		= 0x04; // block
static uLEB128_t const DW_FORM_data2		= 0x05; // constant
static uLEB128_t const DW_FORM_data4		= 0x06; // constant
static uLEB128_t const DW_FORM_data8		= 0x07; // constant
static uLEB128_t const DW_FORM_string		= 0x08; // string
static uLEB128_t const DW_FORM_block		= 0x09; // block
static uLEB128_t const DW_FORM_block1		= 0x0a; // block
static uLEB128_t const DW_FORM_data1		= 0x0b; // constant
static uLEB128_t const DW_FORM_flag		= 0x0c; // flag
static uLEB128_t const DW_FORM_sdata		= 0x0d; // constant
static uLEB128_t const DW_FORM_strp		= 0x0e; // string
static uLEB128_t const DW_FORM_udata		= 0x0f; // constant
static uLEB128_t const DW_FORM_ref_addr		= 0x10; // reference
static uLEB128_t const DW_FORM_ref1		= 0x11; // reference
static uLEB128_t const DW_FORM_ref2		= 0x12; // reference
static uLEB128_t const DW_FORM_ref4		= 0x13; // reference
static uLEB128_t const DW_FORM_ref8		= 0x14; // reference
static uLEB128_t const DW_FORM_ref_udata	= 0x15; // reference
static uLEB128_t const DW_FORM_indirect		= 0x16; // (see section 7.5.3)

#if DEBUGDWARF
char const* print_DW_FORM_name(uLEB128_t form)
{
  switch(form)
  {
    case DW_FORM_addr: return "DW_FORM_addr";
    case DW_FORM_block2: return "DW_FORM_block2";
    case DW_FORM_block4: return "DW_FORM_block4";
    case DW_FORM_data2: return "DW_FORM_data2";
    case DW_FORM_data4: return "DW_FORM_data4";
    case DW_FORM_data8: return "DW_FORM_data8";
    case DW_FORM_string: return "DW_FORM_string";
    case DW_FORM_block: return "DW_FORM_block";
    case DW_FORM_block1: return "DW_FORM_block1";
    case DW_FORM_data1: return "DW_FORM_data1";
    case DW_FORM_flag: return "DW_FORM_flag";
    case DW_FORM_sdata: return "DW_FORM_sdata";
    case DW_FORM_strp: return "DW_FORM_strp";
    case DW_FORM_udata: return "DW_FORM_udata";
    case DW_FORM_ref_addr: return "DW_FORM_ref_addr";
    case DW_FORM_ref1: return "DW_FORM_ref1";
    case DW_FORM_ref2: return "DW_FORM_ref2";
    case DW_FORM_ref4: return "DW_FORM_ref4";
    case DW_FORM_ref8: return "DW_FORM_ref8";
    case DW_FORM_ref_udata: return "DW_FORM_ref_udata";
    case DW_FORM_indirect: return "DW_FORM_indirect";
    case 0: return "0";
  }
  return "UNKNOWN DW_FORM";
}
#endif

// Standard opcodes.
static unsigned char const DW_LNS_copy			= 1;
static unsigned char const DW_LNS_advance_pc		= 2;
static unsigned char const DW_LNS_advance_line		= 3;
static unsigned char const DW_LNS_set_file		= 4;
static unsigned char const DW_LNS_set_column		= 5;
static unsigned char const DW_LNS_negate_stmt		= 6;
static unsigned char const DW_LNS_set_basic_block	= 7;
static unsigned char const DW_LNS_const_add_pc		= 8;
static unsigned char const DW_LNS_fixed_advance_pc	= 9;

// Extended opcodes.
static uLEB128_t const DW_LNE_end_sequence	= 1;
static uLEB128_t const DW_LNE_set_address	= 2;
static uLEB128_t const DW_LNE_define_file	= 3;

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

#if DEBUGSTABS || DEBUGDWARF
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
  bool M_debug_info_loaded;
  Elf32_Word M_stabs_section_index;
  Elf32_Word M_dwarf_debug_info_section_index;
  Elf32_Word M_dwarf_debug_abbrev_section_index;
  Elf32_Word M_dwarf_debug_line_section_index;
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
  void load_dwarf(void);
  template<typename T>
    inline void object_file_ct::dwarf_read(unsigned char const*& debug_info_ptr, T& x);
};

//-------------------------------------------------------------------------------------------------------------------------------------------
// Implementation

static asection_st const abs_section = { 0, "*ABS*" };
asection_st const* const absolute_section = &abs_section;

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
    Dout(dc::bfd, "Object file has non-native data encoding.");
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
      if (DEBUGELF32)
	Dout(dc::bfd, "Found symbol table " << M_sections[i].name << " with " << number_of_symbols << " symbols.");
      Elf32_sym* symbols = (Elf32_sym*)allocate_and_read_section(i);
      for(int s = 0; s < number_of_symbols; ++s)
      {
	Elf32_sym& symbol(symbols[s]);
	if (M_sections[i].section_header().sh_type == SHT_SYMTAB)
	  new_symbol->name = &M_symbol_string_table[symbol.st_name];
	else
	  new_symbol->name = &M_dyn_symbol_string_table[symbol.st_name];
	if (!*new_symbol->name)
	  continue;								// Skip Symbols that do not have a name.
	if (symbol.st_shndx == SHN_ABS)
	{
	  // Skip all absolute symbols except "_end".
	  if (new_symbol->name[1] != 'e' || new_symbol->name[0] != '_' || new_symbol->name[2] != 'n' || new_symbol->name[3] != 'd' || new_symbol->name[4] != 0)
	    continue;
	  new_symbol->section = absolute_section;
          M_s_end_vma = new_symbol->value = symbol.st_value;
	}
        else if (symbol.st_shndx >= SHN_LORESERVE || symbol.st_shndx == SHN_UNDEF)
	  continue;							// Skip Special Sections and Undefined Symbols.
	else
	{
	  new_symbol->section = &M_sections[symbol.st_shndx];
	  new_symbol->value = symbol.st_value - new_symbol->section->vma;	// Is not an absolute value: make value relative
	  									// to start of section.
          if (DEBUGELF32)
	    Dout(dc::bfd, "Symbol \"" << new_symbol->name << "\" in section \"" << new_symbol->section->name << "\".");
        }
	new_symbol->bfd_ptr = this;
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

template<typename T>
  inline void object_file_ct::dwarf_read(unsigned char const*& in, T& x)
  {
    x = *reinterpret_cast<T const*>(in);
    in += sizeof(T);
  }

template<>
  void object_file_ct::dwarf_read(unsigned char const*& in, uLEB128_t& x)
  {
    int shift = 7;
    uLEB128_t byte = *in;
    x = byte;
    while(byte >= 0x80)
    {
      byte = (*++in) ^ 1;
      ASSERT( byte < (1UL << (number_of_bits_in_uLEB128_t - shift)) );
      x ^= byte << shift;
      shift += 7;
    }
    ++in;
  }

template<>
  void object_file_ct::dwarf_read(unsigned char const*& in, LEB128_t& x)
  {
    int shift = 7;
    LEB128_t byte = *in;
    x = byte;
    while(byte >= 0x80)
    {
      byte = (*++in) ^ 1;
      ASSERT( byte < (1L << (number_of_bits_in_LEB128_t - shift)) );
      x ^= byte << shift;
      shift += 7;
    }
    if (shift < number_of_bits_in_LEB128_t && (byte & 0x40))
      x |= - (1L << shift);
    ++in;
  }

struct attr_st {
  uLEB128_t attr;
  uLEB128_t form;
};

struct abbrev_st {
  uLEB128_t code;
  uLEB128_t tag;
  attr_st* attributes;
  unsigned short attributes_size;
  unsigned short attributes_capacity;
  bool has_children;
  abbrev_st(void) : attributes(NULL), attributes_size(0), attributes_capacity(0) { }
  ~abbrev_st() { free(attributes); }
};

struct file_name_st {
  char const* name;
  uLEB128_t directory_index;
  uLEB128_t time_of_last_modification;
  uLEB128_t length_in_bytes_of_the_file;
};

void object_file_ct::load_dwarf(void)
{
  uint32_t total_length;
  if (DEBUGDWARF)
  {
    // Not loaded already.
    ASSERT( !M_debug_info_loaded && M_dwarf_debug_line_section_index != 0 );
    // Don't have a fixed entry sizes.
    ASSERT( M_sections[M_dwarf_debug_line_section_index].section_header().sh_entsize == 0 );
    ASSERT( M_sections[M_dwarf_debug_info_section_index].section_header().sh_entsize == 0 );
    ASSERT( M_sections[M_dwarf_debug_abbrev_section_index].section_header().sh_entsize == 0 );
    // Initialization of debug variable.
    total_length = 0;
  }

  // Start of .debug_abbrev section.
  unsigned char* debug_abbrev = (unsigned char*)allocate_and_read_section(M_dwarf_debug_abbrev_section_index);
  // Start of .debug_info section.
  unsigned char* debug_info = (unsigned char*)allocate_and_read_section(M_dwarf_debug_info_section_index);
  unsigned char* debug_info_end = debug_info + M_sections[M_dwarf_debug_info_section_index].section_header().sh_size;
  // Start of .debug_line section.
  unsigned char* debug_line = (unsigned char*)allocate_and_read_section(M_dwarf_debug_line_section_index);
  unsigned char const* debug_line_ptr;

  // Run over all compilation units.
  for (unsigned char const* debug_info_ptr = debug_info; debug_info_ptr < debug_info_end;)
  {
    uint32_t length;
    dwarf_read(debug_info_ptr, length);
    if (DEBUGDWARF)
    {
      Dout(dc::bfd, "debug_info_ptr = " << (void*)debug_info_ptr << "; debug_info_end = " << (void*)debug_info_end);
      Dout(dc::bfd, "length = " << length);
      total_length += length + 4;
      Dout(dc::bfd, "total length = " << total_length << " (of " <<
	  M_sections[M_dwarf_debug_info_section_index].section_header().sh_size << ").");
      ASSERT( total_length <= M_sections[M_dwarf_debug_info_section_index].section_header().sh_size );
    }
    uint16_t version;
    dwarf_read(debug_info_ptr, version);
    if ( version == 2 )		// DWARF version 2
    {
      uint32_t abbrev_offset;
      dwarf_read(debug_info_ptr, abbrev_offset);
      unsigned char const* debug_abbrev_ptr = debug_abbrev + abbrev_offset;
      DoutDwarf(dc::bfd, "abbrev_offset = " << std::hex << abbrev_offset);
      unsigned char address_size;
      dwarf_read(debug_info_ptr, address_size);
      ASSERT( address_size == sizeof(void*) );

      unsigned int expected_code = 1;
      std::vector<abbrev_st> abbrev_entries(256);
      while(true)
      {
	if (expected_code >= abbrev_entries.size())
	  abbrev_entries.resize(abbrev_entries.size() + 256);
	abbrev_st& abbrev(abbrev_entries[expected_code]);

	dwarf_read(debug_abbrev_ptr, abbrev.code);
	if (abbrev.code == 0)
	  break;
	DoutDwarf(dc::bfd, "code: " << abbrev.code);
	ASSERT( abbrev.code == expected_code );
	++expected_code;

	dwarf_read(debug_abbrev_ptr, abbrev.tag);
	DoutDwarf(dc::bfd, "Tag : " << print_DW_TAG_name(abbrev.tag));
	unsigned char children;
	dwarf_read(debug_abbrev_ptr, children);
	abbrev.has_children = (children == DW_CHILDREN_yes);
	DoutDwarf(dc::bfd, "Has children: " << abbrev.has_children);

        while(true)
	{
	  if (abbrev.attributes_size == abbrev.attributes_capacity)
	  {
	    abbrev.attributes_capacity += 32;
	    abbrev.attributes = (attr_st*)realloc(abbrev.attributes, abbrev.attributes_capacity * sizeof(attr_st));
	  }
	  uLEB128_t& attr(abbrev.attributes[abbrev.attributes_size].attr);
	  uLEB128_t& form(abbrev.attributes[abbrev.attributes_size].form);
	  dwarf_read(debug_abbrev_ptr, attr);
	  dwarf_read(debug_abbrev_ptr, form);
	  DoutDwarf(dc::bfd, "Attribute/Form: " << print_DW_AT_name(attr) << ", " << print_DW_FORM_name(form));
	  if (attr == 0 && form == 0)
	    break;
	  ++abbrev.attributes_size;
	}
	abbrev.attributes = (attr_st*)realloc(abbrev.attributes, abbrev.attributes_size * sizeof(attr_st));

#ifdef DWARFSPEEDUP
	// We only need the DW_TAG_compile_unit abbreviation and this seems to be always the first entry.
	// In the case that I am mistaken about that, we load it all.
	if (abbrev.tag == DW_TAG_compile_unit && abbrev.code == 1)
	  break;
        Dout(dc::warning, "Please mail libcw@alinoe.com that DW_TAG_compile_unit is not the first abbrev in your case.");
#endif
      }

      int level = 0;
      unsigned char const* debug_info_ptr_end = debug_info_ptr + length - sizeof(version) - sizeof(abbrev_offset) - sizeof(address_size);
      while(true)
      {
	uLEB128_t code;
	dwarf_read(debug_info_ptr, code);
	DoutDwarf(dc::bfd, "code: " << code);
	if (code == 0)
	{
          if (DEBUGDWARF)
	    Debug(
	      std::string marker = libcw_do.get_marker();
	      marker.erase(marker.size() - 1);
	      libcw_do.set_marker(marker);
	    );
	  if (--level <= 0)
	  {
	    if (DEBUGDWARF)
	      ASSERT( level == 0 );
	    break;
          }
	  continue;
	}
	ASSERT( code < abbrev_entries.size() );
        if (DEBUGDWARF)
	  Debug(libcw_do.inc_indent(4));
	abbrev_st& abbrev(abbrev_entries[code]);
	DoutDwarf(dc::bfd, "Tag: " << print_DW_TAG_name(abbrev.tag) << " (children: " << (abbrev.has_children ? "yes" : "no") << ')');
        if (abbrev.has_children)
	{
	  ++level;
          if (DEBUGDWARF)
	    Debug(
	      std::string marker = libcw_do.get_marker();
	      marker += '|';
	      libcw_do.set_marker(marker);
	    );
	}

	std::string default_dir;
	std::string cur_dir;
	std::string default_source;
	bool found_stmt_list = false;

	attr_st* attr = abbrev.attributes;
	for (int i = 0; i < abbrev.attributes_size; ++i, ++attr)
	{
	  uLEB128_t form = attr->form;
	  DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr->attr) << ' ');
	  if (attr->attr == DW_AT_stmt_list)
	  {
	    ASSERT( form == DW_FORM_data4 );
	    uint32_t line_offset;
	    dwarf_read(debug_info_ptr, line_offset);
	    DoutDwarf(dc::finish, "0x" << std::hex << line_offset);
	    debug_line_ptr = debug_line + line_offset;
	    found_stmt_list = true;
	    continue;
          }
	  else if (attr->attr == DW_AT_name)
	  {
	    ASSERT( form == DW_FORM_string );
	    DoutDwarf(dc::finish, '(' << print_DW_FORM_name(form) << ") \"" << reinterpret_cast<char const*>(debug_info_ptr) << '"');
	    if (*debug_info_ptr == '/')
	      default_dir.erase();
            default_source.assign(reinterpret_cast<char const*>(debug_info_ptr));
	    default_source += '\0';
	    while(*debug_info_ptr++);
	    continue;
	  }
	  else if (attr->attr == DW_AT_comp_dir)
	  {
	    ASSERT( form == DW_FORM_string );
	    DoutDwarf(dc::finish, '(' << print_DW_FORM_name(form) << ") \"" << reinterpret_cast<char const*>(debug_info_ptr) << '"');
	    if (*debug_info_ptr != '/')
	    {
	      default_dir.assign(reinterpret_cast<char const*>(debug_info_ptr));
	      default_dir += '/';
	    }
	    while(*debug_info_ptr++);
	    continue;
          }
indirect:
	  DoutDwarf(dc::continued, '(' << print_DW_FORM_name(form) << ") ");
	  switch(form)
	  {
	    case DW_FORM_data1:
	      DoutDwarf(dc::finish, (int)*reinterpret_cast<unsigned char const*>(debug_info_ptr));
	      debug_info_ptr += 1;
	      break;
	    case DW_FORM_data2:
	      DoutDwarf(dc::finish, (int)*reinterpret_cast<unsigned short const*>(debug_info_ptr));
	      debug_info_ptr += 2;
	      break;
	    case DW_FORM_data4:
	      DoutDwarf(dc::finish, *reinterpret_cast<unsigned int const*>(debug_info_ptr));
	      debug_info_ptr += 4;
	      break;
	    case DW_FORM_data8:
	      DoutDwarf(dc::finish, *reinterpret_cast<unsigned long long const*>(debug_info_ptr));
	      debug_info_ptr += 8;
	      break;
	    case DW_FORM_indirect:
	      DoutDwarf(dc::continued, "-> ");
	      dwarf_read(debug_info_ptr, form);
	      goto indirect;
	    case DW_FORM_string:
	      DoutDwarf(dc::finish, "\"" << reinterpret_cast<char const*>(debug_info_ptr) << '"');
	      while(*debug_info_ptr++);
	      break;
	    case DW_FORM_ref1:
	      DoutDwarf(dc::finish, (int)*reinterpret_cast<unsigned char const*>(debug_info_ptr));
	      debug_info_ptr += 1;
	      break;
	    case DW_FORM_ref2:
	      DoutDwarf(dc::finish, (int)*reinterpret_cast<unsigned short const*>(debug_info_ptr));
	      debug_info_ptr += 2;
	      break;
	    case DW_FORM_ref4:
	      DoutDwarf(dc::finish, *reinterpret_cast<unsigned int const*>(debug_info_ptr));
	      debug_info_ptr += 4;
	      break;
	    case DW_FORM_ref8:
	      DoutDwarf(dc::finish, *reinterpret_cast<unsigned long long const*>(debug_info_ptr));
	      debug_info_ptr += 8;
	      break;
	    case DW_FORM_ref_udata:
	    {
	      uLEB128_t udata;
	      dwarf_read(debug_info_ptr, udata);
	      DoutDwarf(dc::finish, udata);
	      break;
	    }
	    case DW_FORM_flag:
	      DoutDwarf(dc::finish, (bool)*reinterpret_cast<unsigned char const*>(debug_info_ptr));
	      debug_info_ptr += 1;
	      break;
	    case DW_FORM_addr:
	      DoutDwarf(dc::finish, *reinterpret_cast<void* const*>(debug_info_ptr));
	      debug_info_ptr += address_size;
	      break;
	    case DW_FORM_block2:
	    {
	      uint16_t length;
	      dwarf_read(debug_info_ptr, length);
	      DoutDwarf(dc::finish, "[" << length << " bytes]");
	      debug_info_ptr += length;
	      break;
	    }
	    case DW_FORM_block4:
	    {
	      uint32_t length;
	      dwarf_read(debug_info_ptr, length);
	      DoutDwarf(dc::finish, "[" << length << " bytes]");
	      debug_info_ptr += length;
	      break;
	    }
	    case DW_FORM_block:
	    {
	      uLEB128_t length;
	      dwarf_read(debug_info_ptr, length);
	      DoutDwarf(dc::finish, "[" << length << " bytes]");
	      debug_info_ptr += length;
	      break;
	    }
	    case DW_FORM_block1:
	    {
	      unsigned char length;
	      dwarf_read(debug_info_ptr, length);
	      DoutDwarf(dc::finish, "[" << (unsigned int)length << " bytes]");
	      debug_info_ptr += length;
	      break;
	    }
	    case DW_FORM_sdata:
	    {
	      LEB128_t sdata;
	      dwarf_read(debug_info_ptr, sdata);
	      DoutDwarf(dc::finish, sdata);
	      break;
	    }
	    case DW_FORM_udata:
	    {
	      uLEB128_t udata;
	      dwarf_read(debug_info_ptr, udata);
	      DoutDwarf(dc::finish, udata);
	      break;
	    }
	    case DW_FORM_ref_addr:
	      DoutDwarf(dc::finish, *reinterpret_cast<void* const*>(debug_info_ptr));
	      debug_info_ptr += address_size;
	      break;
	    case DW_FORM_strp:
	      DoutDwarf(dc::finish, *reinterpret_cast<unsigned int const*>(debug_info_ptr));
	      debug_info_ptr += 4;
	      break;
	  }
	}
        if (DEBUGDWARF)
	  Debug(libcw_do.dec_indent(4));

	if (abbrev.tag == DW_TAG_compile_unit && found_stmt_list)
	{
	  // ===========================================================================================================================17"
	  // State machine.
	  // See paragraph 6.2 of "DWARF Debugging Information Format" document.

	  Elf32_Addr address;		// The program-counter value corresponding to a machine instruction generated by the compiler.
	  uLEB128_t file;		// An unsigned integer indicating the identity of the source file corresponding to a machine
				      // instruction.
	  unsigned int line;		// An unsigned integer indicating a source line number.  Lines are numbered beginning at 1.
				      // The compiler may emit the value 0 in cases where an instruction cannot be attributed to
				      // any source line.
	  uLEB128_t column;		// An unsigned integer indicating a column number within a source line.  Columns are numbered
				      // beginning at 1. The value 0 is reserved to indicate that a statement begins at the left
				      // edge of the line.
	  bool is_stmt;		// A boolean indicating that the current instruction is the beginning of a statement.
	  bool basic_block;		// A boolean indicating that the current instruction is the beginning of a basic block.
	  bool end_sequence;		// A boolean indicating that the current address is that of the first byte after the end of
				      // a sequence of target machine instructions.
	  uint32_t total_length;	// The size in bytes of the statement information for this compilation unit (not including
				      // the total_length field itself).
	  uint16_t version;		// Version identifier for the statement information format.
	  uint32_t prologue_length;	// The number of bytes following the prologue_length field to the beginning of the first
				      // byte of the statement program itself.
	  unsigned char minimum_instruction_length;	// The size in bytes of the smallest target machine instruction.  Statement
				      // program opcodes that alter the address register first multiply their operands by this value.
	  unsigned char default_is_stmt;	// The initial value of the is_stmt register.
	  char line_base;		// This parameter affects the meaning of the special opcodes.
	  unsigned char line_range;	// This parameter affects the meaning of the special opcodes.
	  unsigned char opcode_base;	// The number assigned to the first special opcode.
	  unsigned char const* standard_opcode_lengths;
	  std::vector<char const*> include_directories;
	  std::vector<file_name_st> file_names;
	  dwarf_read(debug_line_ptr, total_length);
	  unsigned char const* debug_line_ptr_end = debug_line_ptr + total_length;
	  dwarf_read(debug_line_ptr, version);
	  dwarf_read(debug_line_ptr, prologue_length);
	  unsigned char const* statement_program_start = debug_line_ptr + prologue_length;
	  dwarf_read(debug_line_ptr, minimum_instruction_length);
	  dwarf_read(debug_line_ptr, default_is_stmt);
	  dwarf_read(debug_line_ptr, line_base);
	  dwarf_read(debug_line_ptr, line_range);
	  dwarf_read(debug_line_ptr, opcode_base);
	  standard_opcode_lengths = debug_line_ptr;
	  debug_line_ptr += (opcode_base - 1);
	  while(*debug_line_ptr)
	  {
	    DoutDwarf(dc::bfd, "Include directory: " << reinterpret_cast<char const*>(debug_line_ptr));
	    include_directories.push_back(reinterpret_cast<char const*>(debug_line_ptr));
	    while(*debug_line_ptr++);
	  }
	  ++debug_line_ptr;
	  while(true)
	  {
	    if (DEBUGDWARF)
	      ASSERT( debug_line_ptr < statement_program_start );
	    file_name_st file_name;
	    file_name.name = reinterpret_cast<char const*>(debug_line_ptr);
	    if (!*debug_line_ptr++)
	      break;
	    while(*debug_line_ptr++);
	    dwarf_read(debug_line_ptr, file_name.directory_index);
	    dwarf_read(debug_line_ptr, file_name.time_of_last_modification);
	    dwarf_read(debug_line_ptr, file_name.length_in_bytes_of_the_file);
	    DoutDwarf(dc::bfd, "File name: " << file_name.name);
	    file_names.push_back(file_name);
	  }
	  ASSERT( debug_line_ptr == statement_program_start );

	  std::string cur_dir;
	  std::string cur_source;
	  location_st location;
	  range_st range;

	  std::string cur_func("-DWARF symbol\0");	// We don't add function names - this is used to see we're
	  						// doing DWARF in find_nearest_line().
	  location.func_iter = M_function_names.insert(cur_func).first;

	  do
	  {
	    address = 0;
	    file = 0;		// One less than the `file' mentioned in the documentation.
	    line = 1;
	    column = 0;
	    is_stmt = default_is_stmt;
	    basic_block = false;
	    end_sequence = false;

	    location.line = 0;
	    range.start = 0;

	    cur_dir = default_dir;
	    cur_source = cur_dir + default_source;
	    location.source_iter = M_source_files.insert(cur_source).first;

	    while(!end_sequence)
	    {
	      unsigned char opcode;
	      dwarf_read(debug_line_ptr, opcode);
	      if (opcode < opcode_base)					// Standard opcode?
	      {
		switch(opcode)
		{
		  case 0:						// Extended opcode.
		  {
		    uLEB128_t size;					// Size in bytes.
		    dwarf_read(debug_line_ptr, size);
		    ASSERT( size > 0 );
		    uLEB128_t extended_opcode;
		    dwarf_read(debug_line_ptr, extended_opcode);
		    ASSERT( extended_opcode < 0x80 );			// Then it's size is one:
		    --size;
		    switch(extended_opcode)
		    {
		      case DW_LNE_end_sequence:
			ASSERT( size == 0 );
			end_sequence = true;
			DoutDwarf(dc::bfd, "DW_LNE_end_sequence: Address: 0x" << std::hex << address);
			range.size = address - range.start;
			if (location.line)
			  register_range(location, range);
			break;
		      case DW_LNE_set_address:
			ASSERT( size == sizeof(address) );
			dwarf_read(debug_line_ptr, address);
			DoutDwarf(dc::bfd, "DW_LNE_set_address: 0x" << std::hex << address);
			if (!range.start)
			  range.start = address;
			break;
		      case DW_LNE_define_file:
		      {
			unsigned char const* end = debug_line_ptr + size;
			file_name_st file_name;
			file_name.name = reinterpret_cast<char const*>(debug_line_ptr);
			while(*debug_line_ptr++);
			dwarf_read(debug_line_ptr, file_name.directory_index);
			dwarf_read(debug_line_ptr, file_name.time_of_last_modification);
			dwarf_read(debug_line_ptr, file_name.length_in_bytes_of_the_file);
			ASSERT( debug_line_ptr == end );
			DoutDwarf(dc::bfd, "DW_LNE_define_file: " << file_name.name);
			file_names.push_back(file_name);
			break;
		      }
		      default:
		      {
			DoutDwarf(dc::bfd, "Unknown extended opcode: " << std::hex << extended_opcode);
			debug_line_ptr += size;
			break;
		      }
		    }
		    break;
		  }
		  case DW_LNS_copy:
		    DoutDwarf(dc::bfd, "DW_LNS_copy: Address/Line: 0x" << std::hex << address << ", " << std::dec << line);
		    if (location.line && location.line != line)
		    {
		      range.size = address - range.start;
		      register_range(location, range);
		      range.start = address;
		    }
		    location.line = line;
		    basic_block = false;
		    break;
		  case DW_LNS_advance_pc:
		  {
		    uLEB128_t address_increment;
		    dwarf_read(debug_line_ptr, address_increment);
		    address += minimum_instruction_length * address_increment;
		    DoutDwarf(dc::bfd, "DW_LNS_advance_pc: 0x" << std::hex << address);
		    if (!range.start)
		      range.start = address;
		    break;
		  }
		  case DW_LNS_advance_line:
		  {
		    LEB128_t line_increment;
		    dwarf_read(debug_line_ptr, line_increment);
		    line += line_increment;
		    DoutDwarf(dc::bfd, "DW_LNS_advance_line: " << line);
		    break;
		  }
		  case DW_LNS_set_file:
		    dwarf_read(debug_line_ptr, file);
		    --file;
		    DoutDwarf(dc::bfd, "DW_LNS_set_file: \"" << file_names[file].name << '"');
		    if (*file_names[file].name == '/')
		      cur_source.assign(file_names[file].name);
		    else
		    {
		      if (file_names[file].directory_index == 0)
			cur_dir = default_dir;
		      else
		      {
			cur_dir.assign(include_directories[file_names[file].directory_index - 1]);
			cur_dir += '/';
		      }
		      cur_source = cur_dir;
		      cur_source.append(file_names[file].name);
		    }
		    cur_source += '\0';
		    location.source_iter = M_source_files.insert(cur_source).first;
		    location.line = 0;
		    break;
		  case DW_LNS_set_column:
		    dwarf_read(debug_line_ptr, column);
		    DoutDwarf(dc::bfd, "DW_LNS_set_column: " << column);
		    break;
		  case DW_LNS_negate_stmt:
		    is_stmt = !is_stmt;
		    DoutDwarf(dc::bfd, "DW_LNS_negate_stmt: " << (is_stmt ? "true" : "false"));
		    break;
		  case DW_LNS_set_basic_block:
		    basic_block = true;
		    DoutDwarf(dc::bfd, "DW_LNS_set_basic_block");
		    break;
		  case DW_LNS_const_add_pc:
		  {
		    unsigned int address_increment = (255 - opcode_base) / line_range;
		    address += minimum_instruction_length * address_increment;
		    DoutDwarf(dc::bfd, "DW_LNS_const_add_pc: 0x" << std::hex << address);
		    if (!range.start)
		      range.start = address;
		    break;
		  }
		  case DW_LNS_fixed_advance_pc:
		  {
		    unsigned short int address_increment;
		    dwarf_read(debug_line_ptr, address_increment);
		    address += minimum_instruction_length * address_increment;
		    DoutDwarf(dc::bfd, "DW_LNS_fixed_advance_pc: 0x" << std::hex << address);
		    if (!range.start)
		      range.start = address;
		    break;
		  }
		  default:
		  {
		    DoutDwarf(dc::bfd, "Unknown standard opcode: " << std::hex << (int)opcode);
		    uLEB128_t argument;
		    for (int n = standard_opcode_lengths[opcode - 1]; n > 0; --n)
		      dwarf_read(debug_line_ptr, argument);
		    break;
		  }
		}
	      }
	      else
	      {
		// Special opcode.
		int line_increment = line_base + ((opcode - opcode_base) % line_range);
		line += line_increment;
		unsigned int address_increment = (opcode - opcode_base) / line_range;
		address += minimum_instruction_length * address_increment;
		DoutDwarf(dc::bfd, "Special opcode.  Address/Line: 0x" << std::hex << address << ", " << std::dec << line);
		if (location.line && location.line != line)	// Catenate ranges with same location.
		{
		  range.size = address - range.start;
		  register_range(location, range);
		  range.start = address;
		}
		location.line = line;
		basic_block = false;
	      }
	    }
	  }
	  while( debug_line_ptr < debug_line_ptr_end );
	  ASSERT( debug_line_ptr == debug_line_ptr_end );

	  // End state machine code.
	  // ===========================================================================================================================17"
	}

#ifdef DWARFSPEEDUP
	// No need to read more.
        if (abbrev.tag == DW_TAG_compile_unit)
	{
          if (DEBUGDWARF && level > 0)	// Will be 1, but whatever.
	    Debug(
	      std::string marker = libcw_do.get_marker();
	      marker.erase(marker.size() - level);
	      libcw_do.set_marker(marker);
	    );
	  break;
	}
#endif
      }
#ifdef DWARFSPEEDUP
      // We didn't read till the end (see break above).
      debug_info_ptr = debug_info_ptr_end;
#else
      ASSERT( debug_info_ptr == debug_info_ptr_end );
#endif
    }
    else
    {
      Dout(dc::warning, "DWARF version " << version << " is not understood by libcwd.");
      debug_info_ptr += length - sizeof(version);
    }
  }

  delete [] debug_line;
  delete [] debug_info;
  delete [] debug_abbrev;
  M_debug_info_loaded = true;
}

void object_file_ct::load_stabs(void)
{
  if (DEBUGSTABS)
  {
    ASSERT( !M_debug_info_loaded && M_stabs_section_index != 0 );
    ASSERT( M_sections[M_stabs_section_index].section_header().sh_entsize == sizeof(stab_st) );
  }
  stab_st* stabs = (stab_st*)allocate_and_read_section(M_stabs_section_index);
  if (DEBUGSTABS)
  {
    ASSERT( !strcmp(&M_section_header_string_table[M_sections[M_sections[M_stabs_section_index].section_header().sh_link].section_header().sh_name], ".stabstr") );
    ASSERT( stabs->n_desc == (Elf32_Half)(M_sections[M_stabs_section_index].section_header().sh_size / M_sections[M_stabs_section_index].section_header().sh_entsize - 1) );
  }
  char* stabs_string_table = allocate_and_read_section(M_sections[M_stabs_section_index].section_header().sh_link);
  if (DEBUGSTABS)
    Debug( libcw_do.inc_indent(4) );
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
        if (DEBUGSTABS)
	  Dout(dc::bfd, ((stabs[j].n_type  == N_SO) ? "N_SO : \"" : "N_SOL: \"") << cur_source.data() << "\".");
	break;
      }
      case N_FUN:
      {
	if (stabs[j].n_strx == 0)
	{
          if (DEBUGSTABS)
	    Dout(dc::bfd, "N_FUN: " << "end at " << std::hex << stabs[j].n_value << '.');
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
          if (DEBUGSTABS)
	    Dout(dc::bfd, "N_FUN: " << std::hex << func_addr << " : \"" << &stabs_string_table[stabs[j].n_strx] << "\".");
	}
	break;
      }
      case N_SLINE:
        if (DEBUGSTABS)
	  Dout(dc::bfd, "N_SLINE: " << stabs[j].n_desc << " at " << std::hex << stabs[j].n_value << '.');
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
  if (DEBUGSTABS)
    Debug( libcw_do.dec_indent(4) );
  delete [] stabs;
  delete [] stabs_string_table;
  M_debug_info_loaded = true;
}

void object_file_ct::find_nearest_line(asymbol_st const* symbol, Elf32_Addr offset, char const** file, char const** func, unsigned int* line)
{
  if (!M_debug_info_loaded)
  {
    if (M_dwarf_debug_line_section_index)
      load_dwarf();
    else if (M_stabs_section_index)
      load_stabs();
  }
  range_st range;
  range.start = offset;
  range.size = 1;
  std::map<range_st, location_st, compare_range_st>::const_iterator i(M_ranges.find(static_cast<range_st const>(range)));
  if (i == M_ranges.end() || (*(*(*i).second.func_iter).data() != '-' && strcmp((*(*i).second.func_iter).data(), symbol->name)))
  {
    *file = NULL;
    *func = symbol->name;
    *line = 0;
  }
  else
  {
    *file = (*(*i).second.source_iter).data();
    if (*(*(*i).second.func_iter).data() != '-')	// '-' is used for DWARF symbols by load_dwarf() (see above).
      *func = (*(*i).second.func_iter).data();
    else
      *func = symbol->name;
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
  if ((DEBUGDWARF && M_dwarf_debug_line_section_index)
      || (DEBUGSTABS && M_stabs_section_index))
    Dout(dc::bfd, std::hex << range.start << " - " << (range.start + range.size)
	<< "; " << (*location.source_iter).data() << ':' << std::dec << location.line << " : \""
	<< (*location.func_iter).data() << "\".");
#if DEBUGSTABS || DEBUGDWARF
  std::pair<std::map<range_st, location_st, compare_range_st>::iterator, bool> p(
#endif
      M_ranges.insert(std::pair<range_st, location_st>(range, location))
#if DEBUGSTABS || DEBUGDWARF
      )
#endif
      ;
#if DEBUGSTABS || DEBUGDWARF
  if (!p.second)
  {
    if ((*p.first).second.func_iter != location.func_iter)
      Dout(dc::bfd, "WARNING: Collision between different functions (" << *p.first << ")!?");
    else
    {
      if ((*p.first).first.start != range.start )
        Dout(dc::bfd, "WARNING: Different start for same function (" << *p.first << ")!?");
      if ((*p.first).first.size != range.size)
	Dout(dc::bfd, "WARNING: Different sizes for same function.  Not sure what .stabs entry to use.");
      if ((*p.first).second.line != location.line)
        Dout(dc::bfd, "WARNING: Different line numbers for overlapping range (" << *p.first << ")!?");
      if ((*p.first).second.source_iter != location.source_iter)
        Dout(dc::bfd, "Collision with " << *p.first << ".");
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
    DoutFatal(dc::fatal|error_cf, "std::fstream.open(\"" << file_name << "\")");
  M_input_stream >> M_header;
  ASSERT(M_header.e_shentsize == sizeof(Elf32_Shdr));
  if (M_header.e_shoff == 0 || M_header.e_shnum == 0)
    return;
  M_input_stream.rdbuf()->pubseekpos(M_header.e_shoff);
  Elf32_Shdr* section_headers = new Elf32_Shdr [M_header.e_shnum];
  M_input_stream.read(reinterpret_cast<char*>(section_headers), M_header.e_shnum * sizeof(Elf32_Shdr));
  if (DEBUGELF32)
    Dout(dc::bfd, "Number of section headers: " << M_header.e_shnum);
  ASSERT( section_headers[M_header.e_shstrndx].sh_size > 0
      && section_headers[M_header.e_shstrndx].sh_size >= section_headers[M_header.e_shstrndx].sh_name );
  M_section_header_string_table = new char[section_headers[M_header.e_shstrndx].sh_size]; 
  M_input_stream.rdbuf()->pubseekpos(section_headers[M_header.e_shstrndx].sh_offset);
  M_input_stream.read(M_section_header_string_table, section_headers[M_header.e_shstrndx].sh_size);
  ASSERT( !strcmp(&M_section_header_string_table[section_headers[M_header.e_shstrndx].sh_name], ".shstrtab") );
  M_sections = new section_ct[M_header.e_shnum];
  if (DEBUGELF32)
    Debug( libcw_do.inc_indent(4) );
  M_debug_info_loaded = false;
  M_stabs_section_index = 0;
  M_dwarf_debug_line_section_index = 0;
  for(int i = 0; i < M_header.e_shnum; ++i)
  {
    if (DEBUGELF32 && section_headers[i].sh_name)
      Dout(dc::bfd, "Section name: \"" << &M_section_header_string_table[section_headers[i].sh_name] << '"');
    M_sections[i].init(M_section_header_string_table, section_headers[i]);
    if (!strcmp(M_sections[i].name, ".strtab"))
      M_symbol_string_table = allocate_and_read_section(i);
    else if (!strcmp(M_sections[i].name, ".dynstr"))
      M_dyn_symbol_string_table = allocate_and_read_section(i);
    else if (M_dwarf_debug_line_section_index == 0 && !strcmp(M_sections[i].name, ".stab"))
      M_stabs_section_index = i;
    else if (!strcmp(M_sections[i].name, ".debug_line"))
    {
      M_dwarf_debug_line_section_index = i;
      M_stabs_section_index = 0;	// Use DWARF if available.
    }
    else if (!strcmp(M_sections[i].name, ".debug_abbrev"))
      M_dwarf_debug_abbrev_section_index = i;
    else if (!strcmp(M_sections[i].name, ".debug_info"))
      M_dwarf_debug_info_section_index = i;
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
  if (DEBUGELF32)
  {
    Debug( libcw_do.dec_indent(4) );
    Dout(dc::bfd, "Number of symbols: " << M_number_of_symbols);
  }
  delete [] section_headers;

  // Temporally preload them here.
  if (M_dwarf_debug_line_section_index)
    load_dwarf();
  else if (M_stabs_section_index)
    load_stabs();
}

} // namespace elf32

  } // namespace debug
} // namespace libcw

#endif // !DEBUGUSEGNULIBBFD
