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

//
// This file contains code that reads the symbol table and debug information from
// ELF32/ELF64 object files.
//

#include "sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include <inttypes.h>	// ISO C99 header, needed for int32_t etc.
#include <iomanip>
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include <cerrno>
#include <cstdlib>
#include "cwd_debug.h"
#include "elfxx.h"
#include <libcwd/private_assert.h>
#include "cwd_bfd.h"
#include "compilation_unit.h"

#define DEBUGELFXX 0
#define DEBUGSTABS 0
#define DEBUGDWARF 0

#if DEBUGELFXX || DEBUGSTABS || DEBUGDWARF
static bool const default_dout_c = true;
#endif

#if DEBUGDWARF || DEBUGSTABS
static bool doutdwarfon = default_dout_c;
static bool doutstabson = default_dout_c;
void debug_load_object_file(char const* filename, bool shared);
#endif

#if DEBUGDWARF
#define DoutDwarf(cntrl, x) do { if (doutdwarfon) { _private_::set_alloc_checking_on(LIBCWD_TSD); Dout(cntrl, x); _private_::set_alloc_checking_off(LIBCWD_TSD); } } while(0)
#define DEBUGDWARF_OPT_COMMA(x) ,x
#define DEBUGDWARF_OPT(x) x
#else
#define DoutDwarf(cntrl, x) do { } while(0)
#define DEBUGDWARF_OPT_COMMA(x)
#define DEBUGDWARF_OPT(x)
#endif

#if DEBUGSTABS
#define DoutStabs(cntrl, x) do { if (doutstabson) { _private_::set_alloc_checking_on(LIBCWD_TSD); Dout(cntrl, x); _private_::set_alloc_checking_off(LIBCWD_TSD); } } while(0)
#else
#define DoutStabs(cntrl, x) do { } while(0)
#endif

#if DEBUGELFXX
static bool doutelfxxon = default_dout_c;
#define DoutElfxx(cntrl, x) do { if (doutelfxxon) { _private_::set_alloc_checking_on(LIBCWD_TSD); Dout(cntrl, x); _private_::set_alloc_checking_off(LIBCWD_TSD); } } while(0)
#else
#define DoutElfxx(cntrl, x) do { } while(0)
#endif

namespace libcwd {

namespace elfxx {

#if !defined(__x86_64__) && !defined(__sparc64) && !defined(__ia64__)
typedef Elf32_Ehdr Elfxx_Ehdr;
typedef Elf32_Word Elfxx_Word;
typedef Elf32_Half Elfxx_Half;
typedef Elf32_Addr Elfxx_Addr;
typedef Elf32_Shdr Elfxx_Shdr;
typedef Elf32_Sym Elfxx_Sym;
#define ELFXX_ST_TYPE(var) ELF32_ST_TYPE(var)
#define ELFXX_ST_BIND(var) ELF32_ST_BIND(var)
#else
typedef Elf64_Ehdr Elfxx_Ehdr;
typedef Elf64_Word Elfxx_Word;
typedef Elf64_Half Elfxx_Half;
typedef Elf64_Addr Elfxx_Addr;
typedef Elf64_Shdr Elfxx_Shdr;
typedef Elf64_Sym Elfxx_Sym;
#define ELFXX_ST_TYPE(var) ELF64_ST_TYPE(var)
#define ELFXX_ST_BIND(var) ELF64_ST_BIND(var)
#endif

//==========================================================================================================================================
// The information about ELF (Executable Linkable Format) needed to write this file
// has been obtained from a file called 'ELF.doc.tar.gz'.  You can get this file from
// the net, for example: ftp://ftp.metalab.unc.edu/pub/Linux/GCC/ELF.doc.tar.gz.

std::istream& operator>>(std::istream& is, Elfxx_Ehdr& header)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
  is.read(reinterpret_cast<char*>(&header), sizeof(Elfxx_Ehdr));
  return is;
}

//==========================================================================================================================================
// The information about stabs (Symbol TABleS) was obtained from http://www.informatik.uni-frankfurt.de/doc/texi/stabs_toc.html.
//

// http://www.informatik.uni-frankfurt.de/doc/texi/stabs_6.html#SEC46
struct stab_st {
  Elfxx_Word n_strx;         			// Index into string table of name.
  unsigned char n_type;         		// Type of symbol.
  unsigned char n_other;        		// Misc info (usually empty).
  Elfxx_Half n_desc; 		       		// Description field.
  Elfxx_Addr n_value;              		// Value of symbol.
};

// Type of symbol, n_type.
//static unsigned char const N_GSYM = 0x20;
//static unsigned char const N_FNAME = 0x22;
static unsigned char const N_FUN = 0x24;
//static unsigned char const N_STSYM = 0x26;
//static unsigned char const N_LCSYM = 0x28;
//static unsigned char const N_MAIN = 0x2a;
//static unsigned char const N_PC = 0x30;
//static unsigned char const N_NSYMS = 0x32;
//static unsigned char const N_NOMAP = 0x34;
//static unsigned char const N_OBJ = 0x38;
//static unsigned char const N_OPT = 0x3c;
//static unsigned char const N_RSYM = 0x40;
//static unsigned char const N_M2C = 0x42;
static unsigned char const N_SLINE = 0x44;
//static unsigned char const N_DSLINE = 0x46;
//static unsigned char const N_BSLINE = 0x48;
//static unsigned char const N_BROWS = 0x48;
//static unsigned char const N_DEFD = 0x4a;
//static unsigned char const N_EHDECL = 0x50;
//static unsigned char const N_MOD2 = 0x50;
//static unsigned char const N_CATCH = 0x54;
//static unsigned char const N_SSYM = 0x60;
static unsigned char const N_SO = 0x64;
//static unsigned char const N_LSYM = 0x80;
//static unsigned char const N_BINCL = 0x82;
static unsigned char const N_SOL = 0x84;
//static unsigned char const N_PSYM = 0xa0;
//static unsigned char const N_EINCL = 0xa2;
//static unsigned char const N_ENTRY = 0xa4;
static unsigned char const N_LBRAC = 0xc0;
//static unsigned char const N_EXCL = 0xc2;
//static unsigned char const N_SCOPE = 0xc4;
static unsigned char const N_RBRAC = 0xe0;
//static unsigned char const N_BCOMM = 0xe2;
//static unsigned char const N_ECOMM = 0xe4;
//static unsigned char const N_ECOML = 0xe8;
//static unsigned char const N_NBTEXT = 0xF0;
//static unsigned char const N_NBDATA = 0xF2;
//static unsigned char const N_NBBSS = 0xF4;
//static unsigned char const N_NBSTS = 0xF6;
//static unsigned char const N_NBLCS = 0xF8;
//static unsigned char const N_LENG = 0xfe;

//==========================================================================================================================================
// The information about DWARF was obtained from http://www.eagercon.com/dwarf/dwarf-2.0.0.pdf
// which is one of the worsed documentations I ever saw :/.
//

class LEB128_t {
private:
  long M_val;
public:
  LEB128_t() { }
  LEB128_t(LEB128_t const& leb) : M_val(leb.M_val) { }
  LEB128_t(long val) : M_val(val) { }
  LEB128_t& operator=(long val) { M_val = val; return *this; }
  operator long() const { return M_val; }
  long value() const { return M_val; }
};

class uLEB128_t {
private:
  unsigned long M_val;
public:
  uLEB128_t() { }
  uLEB128_t(uLEB128_t const& leb) : M_val(leb.M_val) { }
  uLEB128_t(unsigned long val) : M_val(val) { }
  uLEB128_t& operator=(unsigned long val) { M_val = val; return *this; }
  operator unsigned long() const { return M_val; }
  unsigned long value() const { return M_val; }
  uLEB128_t& operator--() { --M_val; return *this; }
};

static int const number_of_bits_in_LEB128_t = 8 * sizeof(LEB128_t);
//static int const number_of_bits_in_uLEB128_t = 8 * sizeof(uLEB128_t);

//static unsigned int const DW_TAG_array_type		= 0x01;
static unsigned int const DW_TAG_class_type		= 0x02;
//static unsigned int const DW_TAG_entry_point		= 0x03;
//static unsigned int const DW_TAG_enumeration_type	= 0x04;
//static unsigned int const DW_TAG_formal_parameter	= 0x05;
//static unsigned int const DW_TAG_imported_declaration	= 0x08;
//static unsigned int const DW_TAG_label			= 0x0a;
static unsigned int const DW_TAG_lexical_block		= 0x0b;
//static unsigned int const DW_TAG_member			= 0x0d;
//static unsigned int const DW_TAG_pointer_type		= 0x0f;
//static unsigned int const DW_TAG_reference_type		= 0x10;
static unsigned int const DW_TAG_compile_unit		= 0x11;
//static unsigned int const DW_TAG_string_type		= 0x12;
static unsigned int const DW_TAG_structure_type		= 0x13;
//static unsigned int const DW_TAG_subroutine_type	= 0x15;
//static unsigned int const DW_TAG_typedef		= 0x16;
//static unsigned int const DW_TAG_union_type		= 0x17;
//static unsigned int const DW_TAG_unspecified_parameters	= 0x18;
//static unsigned int const DW_TAG_variant		= 0x19;
//static unsigned int const DW_TAG_common_block		= 0x1a;
//static unsigned int const DW_TAG_common_inclusion	= 0x1b;
//static unsigned int const DW_TAG_inheritance		= 0x1c;
//static unsigned int const DW_TAG_inlined_subroutine	= 0x1d;
//static unsigned int const DW_TAG_module			= 0x1e;
//static unsigned int const DW_TAG_ptr_to_member_type	= 0x1f;
//static unsigned int const DW_TAG_set_type		= 0x20;
//static unsigned int const DW_TAG_subrange_type		= 0x21;
//static unsigned int const DW_TAG_with_stmt		= 0x22;
//static unsigned int const DW_TAG_access_declaration	= 0x23;
//static unsigned int const DW_TAG_base_type		= 0x24;
//static unsigned int const DW_TAG_catch_block		= 0x25;
//static unsigned int const DW_TAG_const_type		= 0x26;
//static unsigned int const DW_TAG_constant		= 0x27;
//static unsigned int const DW_TAG_enumerator		= 0x28;
//static unsigned int const DW_TAG_file_type		= 0x29;
//static unsigned int const DW_TAG_friend			= 0x2a;
//static unsigned int const DW_TAG_namelist		= 0x2b;
////static unsigned int const DW_TAG_namelist_item		= 0x2c;
//static unsigned int const DW_TAG_packed_type		= 0x2d;
//static unsigned int const DW_TAG_subprogram		= 0x2e;
//static unsigned int const DW_TAG_template_type_param	= 0x2f;
//static unsigned int const DW_TAG_template_value_param	= 0x30;
//static unsigned int const DW_TAG_thrown_type		= 0x31;
//static unsigned int const DW_TAG_try_block		= 0x32;
////static unsigned int const DW_TAG_variant_part		= 0x33;
//static unsigned int const DW_TAG_variable		= 0x34;
//static unsigned int const DW_TAG_volatile_type		= 0x35;
// DWARF 3.
//static unsigned int const DW_TAG_dwarf_procedure	= 0x36;
//static unsigned int const DW_TAG_restrict_type		= 0x37;
//static unsigned int const DW_TAG_interface_type		= 0x38;
static unsigned int const DW_TAG_namespace		= 0x39;
//static unsigned int const DW_TAG_imported_module	= 0x3a;
//static unsigned int const DW_TAG_unspecified_type	= 0x3b;
//static unsigned int const DW_TAG_partial_unit		= 0x3c;
//static unsigned int const DW_TAG_imported_unit		= 0x3d;
//static unsigned int const DW_TAG_condition		= 0x3f;
//static unsigned int const DW_TAG_shared_type		= 0x40;
// DWARF 4.
//static unsigned int const DW_TAG_type_unit		= 0x41;
//static unsigned int const DW_TAG_rvalue_reference_type	= 0x42;
//static unsigned int const DW_TAG_template_alias		= 0x43;
// User range.
//static unsigned int const DW_TAG_lo_user		= 0x4080;
//static unsigned int const DW_TAG_hi_user		= 0xffff;
// SGI/MIPS Extensions.
//static unsigned int const DW_TAG_MIPS_loop		= 0x4081;
// GNU extensions.
//static unsigned int const DW_TAG_format_label		= 0x4101;  // For FORTRAN 77 and Fortran 90.
//static unsigned int const DW_TAG_function_template	= 0x4102;  // For C++.
//static unsigned int const DW_TAG_class_template		= 0x4103;  // For C++.
//static unsigned int const DW_TAG_GNU_BINCL		= 0x4104;
//static unsigned int const DW_TAG_GNU_EINCL		= 0x4105;

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
  }
  switch(tag)
  {
    case DW_TAG_dwarf_procedure: return "DW_TAG_dwarf_procedure";
    case DW_TAG_restrict_type: return "DW_TAG_restrict_type";
    case DW_TAG_interface_type: return "DW_TAG_interface_type";
    case DW_TAG_namespace: return "DW_TAG_namespace";
    case DW_TAG_imported_module: return "DW_TAG_imported_module";
    case DW_TAG_unspecified_type: return "DW_TAG_unspecified_type";
    case DW_TAG_partial_unit: return "DW_TAG_partial_unit";
    case DW_TAG_imported_unit: return "DW_TAG_imported_unit";
    case DW_TAG_condition: return "DW_TAG_condition";
    case DW_TAG_shared_type: return "DW_TAG_shared_type";
  }
  switch(tag)
  {
    case DW_TAG_type_unit: return "DW_TAG_type_unit";
    case DW_TAG_rvalue_reference_type: return "DW_TAG_rvalue_reference_type";
    case DW_TAG_template_alias: return "DW_TAG_template_alias";
  }
  LIBCWD_ASSERT(tag >= DW_TAG_lo_user && tag <= DW_TAG_hi_user);
  switch(tag)
  {
    case DW_TAG_lo_user: return "DW_TAG_lo_user";
    case DW_TAG_hi_user: return "DW_TAG_hi_user";
  }
  switch(tag)
  {
    case DW_TAG_MIPS_loop: return "DW_TAG_MIPS_loop";
  }
  switch(tag)
  {
    case DW_TAG_format_label: return "DW_TAG_format_label";
    case DW_TAG_function_template: return "DW_TAG_function_template";
    case DW_TAG_class_template: return "DW_TAG_class_template";
    case DW_TAG_GNU_BINCL: return "DW_TAG_GNU_BINCL";
    case DW_TAG_GNU_EINCL: return "DW_TAG_GNU_EINCL";
  }
  static char unknown_tag[32];
  sprintf(unknown_tag, "UNKNOWN DW_TAG 0x%lx", tag.value());
  return unknown_tag;
}
#endif

//static unsigned char const DW_CHILDREN_no	= 0;
static unsigned char const DW_CHILDREN_yes	= 1;

static unsigned int const DW_AT_sibling 		= 0x01;	// reference
//static unsigned int const DW_AT_location 		= 0x02;	// block, constant
static unsigned int const DW_AT_name 			= 0x03;	// string
//static unsigned int const DW_AT_ordering 		= 0x09;	// constant
//static unsigned int const DW_AT_byte_size 		= 0x0b;	// constant
//static unsigned int const DW_AT_bit_offset 		= 0x0c;	// constant
//static unsigned int const DW_AT_bit_size 		= 0x0d;	// constant
static unsigned int const DW_AT_stmt_list 		= 0x10;	// constant
static unsigned int const DW_AT_low_pc 			= 0x11;	// address
static unsigned int const DW_AT_high_pc 		= 0x12;	// address
//static unsigned int const DW_AT_language 		= 0x13;	// constant
//static unsigned int const DW_AT_discr 			= 0x15;	// reference
////static unsigned int const DW_AT_discr_value 		= 0x16;	// constant
//static unsigned int const DW_AT_visibility 		= 0x17;	// constant
//static unsigned int const DW_AT_import 			= 0x18;	// reference
//static unsigned int const DW_AT_string_length 		= 0x19;	// block, constant
//static unsigned int const DW_AT_common_reference 	= 0x1a;	// reference
static unsigned int const DW_AT_comp_dir 		= 0x1b;	// string
//static unsigned int const DW_AT_const_value 		= 0x1c;	// string, constant, block
//static unsigned int const DW_AT_containing_type		= 0x1d;	// reference
//static unsigned int const DW_AT_default_value 		= 0x1e;	// reference
//static unsigned int const DW_AT_inline 			= 0x20;	// constant
//static unsigned int const DW_AT_is_optional 		= 0x21;	// flag
//static unsigned int const DW_AT_lower_bound 		= 0x22;	// constant, reference
//static unsigned int const DW_AT_producer 		= 0x25;	// string
//static unsigned int const DW_AT_prototyped 		= 0x27;	// flag
//static unsigned int const DW_AT_return_addr 		= 0x2a;	// block, constant
//static unsigned int const DW_AT_start_scope 		= 0x2c;	// constant
////static unsigned int const DW_AT_stride_size 		= 0x2e;	// constant
//static unsigned int const DW_AT_upper_bound 		= 0x2f;	// constant, reference
//static unsigned int const DW_AT_abstract_origin		= 0x31;	// reference
//static unsigned int const DW_AT_accessibility 		= 0x32;	// constant
//static unsigned int const DW_AT_address_class 		= 0x33;	// constant
//static unsigned int const DW_AT_artificial 		= 0x34;	// flag
//static unsigned int const DW_AT_base_types 		= 0x35;	// reference
//static unsigned int const DW_AT_calling_convention	= 0x36;	// constant
//static unsigned int const DW_AT_count 			= 0x37;	// constant, reference
//static unsigned int const DW_AT_data_member_location	= 0x38; // block, reference
//static unsigned int const DW_AT_decl_column 		= 0x39;	// constant
//static unsigned int const DW_AT_decl_file 		= 0x3a;	// constant
//static unsigned int const DW_AT_decl_line 		= 0x3b;	// constant
//static unsigned int const DW_AT_declaration 		= 0x3c;	// flag
////static unsigned int const DW_AT_discr_list 		= 0x3d;	// block
//static unsigned int const DW_AT_encoding 		= 0x3e;	// constant
//static unsigned int const DW_AT_external 		= 0x3f;	// flag
//static unsigned int const DW_AT_frame_base 		= 0x40;	// block, constant
//static unsigned int const DW_AT_friend 			= 0x41;	// reference
//static unsigned int const DW_AT_identifier_case 	= 0x42;	// constant
//static unsigned int const DW_AT_macro_info 		= 0x43;	// constant
//static unsigned int const DW_AT_namelist_item 		= 0x44;	// block
//static unsigned int const DW_AT_priority 		= 0x45;	// reference
//static unsigned int const DW_AT_segment 		= 0x46;	// block, constant
//static unsigned int const DW_AT_specification 		= 0x47;	// reference
//static unsigned int const DW_AT_static_link 		= 0x48;	// block, constant
//static unsigned int const DW_AT_type 			= 0x49;	// reference
//static unsigned int const DW_AT_use_location 		= 0x4a;	// block, constant
//static unsigned int const DW_AT_variable_parameter	= 0x4b;	// flag
//static unsigned int const DW_AT_virtuality 		= 0x4c;	// constant
//static unsigned int const DW_AT_vtable_elem_location	= 0x4d; // block, reference
// DWARF 3 values.
//static unsigned int const DW_AT_allocated		= 0x4e;
//static unsigned int const DW_AT_associated		= 0x4f;
//static unsigned int const DW_AT_data_location		= 0x50;
//static unsigned int const DW_AT_stride			= 0x51;
//static unsigned int const DW_AT_entry_pc		= 0x52;
//static unsigned int const DW_AT_use_UTF8		= 0x53;
//static unsigned int const DW_AT_extension		= 0x54;
//static unsigned int const DW_AT_ranges			= 0x55;
//static unsigned int const DW_AT_trampoline		= 0x56;
//static unsigned int const DW_AT_call_column		= 0x57;
//static unsigned int const DW_AT_call_file		= 0x58;
//static unsigned int const DW_AT_call_line		= 0x59;
//static unsigned int const DW_AT_description		= 0x5a;
//static unsigned int const DW_AT_binary_scale		= 0x5b;
//static unsigned int const DW_AT_decimal_scale		= 0x5c;
//static unsigned int const DW_AT_small			= 0x5d;
//static unsigned int const DW_AT_decimal_sign		= 0x5e;
//static unsigned int const DW_AT_digit_count		= 0x5f;
//static unsigned int const DW_AT_picture_string		= 0x60;
//static unsigned int const DW_AT_mutable			= 0x61;
//static unsigned int const DW_AT_threads_scaled		= 0x62;
//static unsigned int const DW_AT_explicit		= 0x63;
//static unsigned int const DW_AT_object_pointer		= 0x64;
//static unsigned int const DW_AT_endianity		= 0x65;
//static unsigned int const DW_AT_elemental		= 0x66;
//static unsigned int const DW_AT_pure			= 0x67;
//static unsigned int const DW_AT_recursive		= 0x68;
// DWARF 4 values.
//static unsigned int const DW_AT_signature		= 0x69;
//static unsigned int const DW_AT_main_subprogram		= 0x6a;
//static unsigned int const DW_AT_data_bit_offset		= 0x6b;
//static unsigned int const DW_AT_const_expr		= 0x6c;
//static unsigned int const DW_AT_enum_class		= 0x6d;
//static unsigned int const DW_AT_linkage_name		= 0x6e;
// User range.
//static unsigned int const DW_AT_lo_user				= 0x2000;
//static unsigned int const DW_AT_hi_user				= 0x3fff;
// SGI/MIPS Extensions.
//static unsigned int const DW_AT_MIPS_fde			= 0x2001;
//static unsigned int const DW_AT_MIPS_loop_begin			= 0x2002;
//static unsigned int const DW_AT_MIPS_tail_loop_begin		= 0x2003;
//static unsigned int const DW_AT_MIPS_epilog_begin		= 0x2004;
//static unsigned int const DW_AT_MIPS_loop_unroll_factor		= 0x2005;
//static unsigned int const DW_AT_MIPS_software_pipeline_depth	= 0x2006;
//static unsigned int const DW_AT_MIPS_linkage_name		= 0x2007;
//static unsigned int const DW_AT_MIPS_stride			= 0x2008;
//static unsigned int const DW_AT_MIPS_abstract_name		= 0x2009;
//static unsigned int const DW_AT_MIPS_clone_origin		= 0x200a;
//static unsigned int const DW_AT_MIPS_has_inlines		= 0x200b;
////static unsigned int const DW_AT_MIPS_stride_byte		= 0x200c;
////static unsigned int const DW_AT_MIPS_stride_elem		= 0x200d;
//static unsigned int const DW_AT_MIPS_ptr_dopetype		= 0x200e;
//static unsigned int const DW_AT_MIPS_allocatable_dopetype	= 0x200f;
//static unsigned int const DW_AT_MIPS_assumed_shape_dopetype	= 0x2010;
//static unsigned int const DW_AT_MIPS_assumed_size		= 0x2011;
// GNU extensions.
//static unsigned int const DW_AT_sf_names			= 0x2101;
//static unsigned int const DW_AT_src_info			= 0x2102;
//static unsigned int const DW_AT_mac_info			= 0x2103;
//static unsigned int const DW_AT_src_coords			= 0x2104;
//static unsigned int const DW_AT_body_begin			= 0x2105;
//static unsigned int const DW_AT_body_end			= 0x2106;
//static unsigned int const DW_AT_GNU_vector			= 0x2107;
////static unsigned int const DW_AT_GNU_guarded_by			= 0x2108;
////static unsigned int const DW_AT_GNU_pt_guarded_by		= 0x2109;
//static unsigned int const DW_AT_GNU_guarded			= 0x210a;
//static unsigned int const DW_AT_GNU_pt_guarded			= 0x210b;
//static unsigned int const DW_AT_GNU_locks_excluded		= 0x210c;
//static unsigned int const DW_AT_GNU_exclusive_locks_required	= 0x210d;
//static unsigned int const DW_AT_GNU_shared_locks_required	= 0x210e;
//static unsigned int const DW_AT_GNU_odr_signature		= 0x210f;
//static unsigned int const DW_AT_GNU_template_name		= 0x2110;
//static unsigned int const DW_AT_GNU_call_site_value		= 0x2111;
//static unsigned int const DW_AT_GNU_call_site_data_value	= 0x2112;
//static unsigned int const DW_AT_GNU_call_site_target		= 0x2113;
////static unsigned int const DW_AT_GNU_call_site_target_clobbered	= 0x2114;
//static unsigned int const DW_AT_GNU_tail_call			= 0x2115;
//static unsigned int const DW_AT_GNU_all_tail_call_sites		= 0x2116;
//static unsigned int const DW_AT_GNU_all_call_sites		= 0x2117;
//static unsigned int const DW_AT_GNU_all_source_call_sites	= 0x2118;
//static unsigned int const DW_AT_GNU_macros			= 0x2119;
//static unsigned int const DW_AT_GNU_deleted			= 0x211a;
//static unsigned int const DW_AT_GNU_dwo_name			= 0x2130;
//static unsigned int const DW_AT_GNU_dwo_id			= 0x2131;
//static unsigned int const DW_AT_GNU_ranges_base			= 0x2132;
//static unsigned int const DW_AT_GNU_addr_base			= 0x2133;
//static unsigned int const DW_AT_GNU_pubnames			= 0x2134;
//static unsigned int const DW_AT_GNU_pubtypes			= 0x2135;
// VMS Extensions.
//static unsigned int const DW_AT_VMS_rtnbeg_pd_address		= 0x2201;

#if DEBUGDWARF
char const* print_DW_AT_name(uLEB128_t attr)
{
  switch(attr)
  {
    case 0: return "0";
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
    // DWARF 3 attributes
    case DW_AT_allocated: return "DW_AT_allocated";
    case DW_AT_associated: return "DW_AT_associated";
    case DW_AT_data_location: return "DW_AT_data_location";
    case DW_AT_stride: return "DW_AT_stride";
    case DW_AT_entry_pc: return "DW_AT_entry_pc";
    case DW_AT_use_UTF8: return "DW_AT_use_UTF8";
    case DW_AT_extension: return "DW_AT_extension";
    case DW_AT_ranges: return "DW_AT_ranges";
    case DW_AT_trampoline: return "DW_AT_trampoline";
    case DW_AT_call_column: return "DW_AT_call_column";
    case DW_AT_call_file: return "DW_AT_call_file";
    case DW_AT_call_line: return "DW_AT_call_line";
    case DW_AT_description: return "DW_AT_description";
    case DW_AT_binary_scale: return "DW_AT_binary_scale";
    case DW_AT_decimal_scale: return "DW_AT_decimal_scale";
    case DW_AT_small: return "DW_AT_small";
    case DW_AT_decimal_sign: return "DW_AT_decimal_sign";
    case DW_AT_digit_count: return "DW_AT_digit_count";
    case DW_AT_picture_string: return "DW_AT_picture_string";
    case DW_AT_mutable: return "DW_AT_mutable";
    case DW_AT_threads_scaled: return "DW_AT_threads_scaled";
    case DW_AT_explicit: return "DW_AT_explicit";
    case DW_AT_object_pointer: return "DW_AT_object_pointer";
    case DW_AT_endianity: return "DW_AT_endianity";
    case DW_AT_elemental: return "DW_AT_elemental";
    case DW_AT_pure: return "DW_AT_pure";
    case DW_AT_recursive: return "DW_AT_recursive";
    // DWARF 4 attributes
    case DW_AT_signature: return "DW_AT_signature";
    case DW_AT_main_subprogram: return "DW_AT_main_subprogram";
    case DW_AT_data_bit_offset: return "DW_AT_data_bit_offset";
    case DW_AT_const_expr: return "DW_AT_const_expr";
    case DW_AT_enum_class: return "DW_AT_enum_class";
    case DW_AT_linkage_name: return "DW_AT_linkage_name";
  }
  LIBCWD_ASSERT(attr >= DW_AT_lo_user && attr <= DW_AT_hi_user);
  switch(attr)
  {
    case DW_AT_lo_user: return "DW_AT_lo_user";
    case DW_AT_MIPS_fde: return "DW_AT_MIPS_fde";
    case DW_AT_MIPS_loop_begin: return "DW_AT_MIPS_loop_begin";
    case DW_AT_MIPS_tail_loop_begin: return "DW_AT_MIPS_tail_loop_begin";
    case DW_AT_MIPS_epilog_begin: return "DW_AT_MIPS_epilog_begin";
    case DW_AT_MIPS_loop_unroll_factor: return "DW_AT_MIPS_loop_unroll_factor";
    case DW_AT_MIPS_software_pipeline_depth: return "DW_AT_MIPS_software_pipeline_depth";
    case DW_AT_MIPS_linkage_name: return "DW_AT_MIPS_linkage_name";
    case DW_AT_MIPS_stride: return "DW_AT_MIPS_stride";
    case DW_AT_MIPS_abstract_name: return "DW_AT_MIPS_abstract_name";
    case DW_AT_MIPS_clone_origin: return "DW_AT_MIPS_clone_origin";
    case DW_AT_MIPS_has_inlines: return "DW_AT_MIPS_has_inlines";
  }
  switch(attr)
  {
    case DW_AT_sf_names: return "DW_AT_sf_names";
    case DW_AT_src_info: return "DW_AT_src_info";
    case DW_AT_mac_info: return "DW_AT_mac_info";
    case DW_AT_src_coords: return "DW_AT_src_coords";
    case DW_AT_body_begin: return "DW_AT_body_begin";
    case DW_AT_body_end: return "DW_AT_body_end";
    case DW_AT_GNU_vector: return "DW_AT_GNU_vector";
    case DW_AT_GNU_guarded_by: return "DW_AT_GNU_guarded_by";
    case DW_AT_GNU_pt_guarded_by: return "DW_AT_GNU_pt_guarded_by";
    case DW_AT_GNU_guarded: return "DW_AT_GNU_guarded";
    case DW_AT_GNU_pt_guarded: return "DW_AT_GNU_pt_guarded";
    case DW_AT_GNU_locks_excluded: return "DW_AT_GNU_locks_excluded";
    case DW_AT_GNU_exclusive_locks_required: return "DW_AT_GNU_exclusive_locks_required";
    case DW_AT_GNU_shared_locks_required: return "DW_AT_GNU_shared_locks_required";
    case DW_AT_GNU_odr_signature: return "DW_AT_GNU_odr_signature";
    case DW_AT_GNU_template_name: return "DW_AT_GNU_template_name";
    case DW_AT_GNU_call_site_value: return "DW_AT_GNU_call_site_value";
    case DW_AT_GNU_call_site_data_value: return "DW_AT_GNU_call_site_data_value";
    case DW_AT_GNU_call_site_target: return "DW_AT_GNU_call_site_target";
    case DW_AT_GNU_call_site_target_clobbered: return "DW_AT_GNU_call_site_target_clobbered";
    case DW_AT_GNU_tail_call: return "DW_AT_GNU_tail_call";
    case DW_AT_GNU_all_tail_call_sites: return "DW_AT_GNU_all_tail_call_sites";
    case DW_AT_GNU_all_call_sites: return "DW_AT_GNU_all_call_sites";
    case DW_AT_GNU_all_source_call_sites: return "DW_AT_GNU_all_source_call_sites";
    case DW_AT_GNU_macros: return "DW_AT_GNU_macros";
    case DW_AT_GNU_deleted: return "DW_AT_GNU_deleted";
    case DW_AT_GNU_dwo_name: return "DW_AT_GNU_dwo_name";
    case DW_AT_GNU_dwo_id: return "DW_AT_GNU_dwo_id";
    case DW_AT_GNU_ranges_base: return "DW_AT_GNU_ranges_base";
    case DW_AT_GNU_addr_base: return "DW_AT_GNU_addr_base";
    case DW_AT_GNU_pubnames: return "DW_AT_GNU_pubnames";
    case DW_AT_GNU_pubtypes: return "DW_AT_GNU_pubtypes";
  }
  switch(attr)
  {
    case DW_AT_VMS_rtnbeg_pd_address: return "DW_AT_VMS_rtnbeg_pd_address";
  }
  static char unknown_at[32];
  sprintf(unknown_at, "UNKNOWN DW_AT 0x%lx", attr.value());
  return unknown_at;
}
#endif

static unsigned int const DW_FORM_addr		= 0x01; // address
static unsigned int const DW_FORM_block2	= 0x03; // block
static unsigned int const DW_FORM_block4	= 0x04; // block
static unsigned int const DW_FORM_data2		= 0x05; // constant
static unsigned int const DW_FORM_data4		= 0x06; // constant
static unsigned int const DW_FORM_data8		= 0x07; // constant
static unsigned int const DW_FORM_string	= 0x08; // string
static unsigned int const DW_FORM_block		= 0x09; // block
static unsigned int const DW_FORM_block1	= 0x0a; // block
static unsigned int const DW_FORM_data1		= 0x0b; // constant
static unsigned int const DW_FORM_flag		= 0x0c; // flag
static unsigned int const DW_FORM_sdata		= 0x0d; // constant
static unsigned int const DW_FORM_strp		= 0x0e; // string
static unsigned int const DW_FORM_udata		= 0x0f; // constant
static unsigned int const DW_FORM_ref_addr	= 0x10; // reference
static unsigned int const DW_FORM_ref1		= 0x11; // reference
static unsigned int const DW_FORM_ref2		= 0x12; // reference
static unsigned int const DW_FORM_ref4		= 0x13; // reference
static unsigned int const DW_FORM_ref8		= 0x14; // reference
static unsigned int const DW_FORM_ref_udata	= 0x15; // reference
static unsigned int const DW_FORM_indirect	= 0x16; // (see section 7.5.3)
static unsigned int const DW_FORM_sec_offset	= 0x17; // lineptr, loclistptr, macptr, rangelistptr
static unsigned int const DW_FORM_exprloc	= 0x18; // exprloc
static unsigned int const DW_FORM_flag_present	= 0x19; // flag
static unsigned int const DW_FORM_ref_sig8	= 0x20; // reference

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
    case DW_FORM_sec_offset: return "DW_FORM_sec_offset";
    case DW_FORM_exprloc: return "DW_FORM_exprloc";
    case DW_FORM_flag_present: return "DW_FORM_flag_present";
    case DW_FORM_ref_sig8: return "DW_FORM_ref_sig8";
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
// DWARF 3
//static unsigned char const DW_LNS_set_prologue_end	= 10;
//static unsigned char const DW_LNS_set_epilogue_begin	= 11;
//static unsigned char const DW_LNS_set_isa		= 12;

// Extended opcodes.
static unsigned int const DW_LNE_end_sequence	= 1;
static unsigned int const DW_LNE_set_address	= 2;
static unsigned int const DW_LNE_define_file	= 3;
// DWARF 4
//static unsigned int const DW_LNE_set_discriminator = 4;

static unsigned char address_size;	// Should be sizeof(void*) - at least it is constant,
					// so it's thread safe to be static.

//--------------------------------------------------------------------------------------------------------------------------
// The types (defined in "2.2 Attribute Types" of the draft).

// address	: Refers to some location in the address space of the described program.
typedef Elfxx_Addr address_t;
// block	: An arbitrary number of uninterpreted bytes of data.
struct block_t { unsigned char const* begin; size_t number_of_bytes; };
// constant	: One, two, four or eight bytes of uninterpreted data, or data encoded
//                in the variable length format known as LEB128 (see section 7.6).
// We don't handle 8 byte sizes if uLEB128_t doesn't either.
typedef uLEB128_t constant_t;
// flag		: A small constant that indicates the presence or absence of an attribute.
typedef bool flag_t;
// reference	: Refers to some member of the set of debugging information entries that
//                describe the program.  There are two types of reference.  The first is
//                an offset relative to the beginning of the compilation unit in which the
//                reference occurs and must refer to an entry within that same compilation
//                unit.  The second type of reference is the address of any debugging
//                information entry within the same executable or shared object; it may
//                refer to an entry in a different compilation unit from the unit containing
//                the reference.
typedef unsigned char const* reference_t;
// string	: A null-terminated sequence of zero or more (non-null) bytes.  Data in this
//                form are generally printable strings. Strings may be represented directly
//                in the debugging information entry or as an offset in a separate string table.
typedef char const* string_t;
// lineptr	: Refers to a location in the DWARF section that holds line number information.
typedef unsigned char const* lineptr_t;

//------------------------------------------------

template<typename T>
  static inline T
  reinterpret_cast_align(unsigned char const* in)
  {
    union { T result; unsigned char input[sizeof(T)]; } aligner;
    for (size_t i = 0; i < sizeof(T); ++i)
      aligner.input[i] = in[i];
    return aligner.result;
  }

template<typename T>
  static inline void
  dwarf_read(unsigned char const*& in, T& x)
  {
    x = reinterpret_cast_align<T>(in);
    in += sizeof(T);
  }

template<>
  void
  dwarf_read(unsigned char const*& in, uLEB128_t& x)
  {
    int shift = 7;
    uLEB128_t byte = *in;
    x = byte;
    while(byte >= 0x80)
    {
      ++in;
      byte = (*in) ^ 1;
      // We can't use this assertion because gcc writes constants as
      // signed integers to debug info (because of a bug in gdb version 4)
      // and the sign extension can cause more than 32 bits to be set.
      //LIBCWD_ASSERT( byte < (1UL << (number_of_bits_in_uLEB128_t - shift)));
      x = x.value() ^ (byte.value() << shift);
      shift += 7;
    }
    ++in;
  }

template<>
  void
  dwarf_read(unsigned char const*& in, LEB128_t& x)
  {
    int shift = 7;
    LEB128_t byte = *in;
    x = byte;
    while(byte >= 0x80)
    {
      byte = (*++in) ^ 1;
      LIBCWD_ASSERT( byte < (1L << (number_of_bits_in_LEB128_t - shift)) );
      x = x.value() ^ (byte.value() << shift);
      shift += 7;
    }
    if (shift < number_of_bits_in_LEB128_t && (byte & 0x40))
      x = x.value() | (- (1L << shift));
    ++in;
  }

inline static void
eat_string(unsigned char const*& debug_info_ptr)
{
#ifdef __i386__
  // The optimization of gcc is horrible.  Lets do it ourselfs.
  int __d0;
  asm("cld; repnz; scasb" : "=c" (__d0), "=D" (debug_info_ptr) : "0" (-1), "1" (debug_info_ptr), "a" (0));
#else
  // This is the same as 'while(*debug_info_ptr++);', but that results in horribly
  // inefficient code (tested with g++ 3.4).
  unsigned char const* tmp = debug_info_ptr;
  while(*tmp)
    ++tmp;
  debug_info_ptr = tmp + 1;
#endif
}

//------------------------------------------------
// Conversion routines.
// The following routines read a FORM of the expected type
// and return the value as one of the types given above.

inline address_t
read_address(unsigned char const*& debug_info_ptr, uLEB128_t const DEBUGDWARF_OPT(form))
{
#if DEBUGDWARF
  LIBCWD_ASSERT(form == DW_FORM_addr);
#endif
  address_t result;
  dwarf_read(debug_info_ptr, result);
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  DoutDwarf(dc::finish, result);
#endif
  return result;
}

block_t
read_block(unsigned char const*& debug_info_ptr, uLEB128_t const form)
{
#if DEBUGDWARF
  LIBCWD_ASSERT(form == DW_FORM_block1 || form == DW_FORM_block2 || form == DW_FORM_block4 || form == DW_FORM_block);
#endif
  block_t result;
  result.begin = debug_info_ptr;
  switch(form)
  {
    case DW_FORM_block1:
    {
      uint8_t number_of_bytes;
      dwarf_read(debug_info_ptr, number_of_bytes);
      result.number_of_bytes = number_of_bytes;
      break;
    }
    case DW_FORM_block2:
    {
      uint16_t number_of_bytes;
      dwarf_read(debug_info_ptr, number_of_bytes);
      result.number_of_bytes = number_of_bytes;
      break;
    }
    case DW_FORM_block4:
    {
      uint32_t number_of_bytes;
      dwarf_read(debug_info_ptr, number_of_bytes);
      result.number_of_bytes = number_of_bytes;
      break;
    }
    case DW_FORM_block:
    {
      uLEB128_t number_of_bytes;
      dwarf_read(debug_info_ptr, number_of_bytes);
      result.number_of_bytes = number_of_bytes;
      break;
    }
  }
  debug_info_ptr += result.number_of_bytes;
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  DoutDwarf(dc::finish, "FIXME");
#endif
  return result;
}

inline constant_t
read_constant(unsigned char const*& debug_info_ptr, uLEB128_t const form)
{
#if DEBUGDWARF
  LIBCWD_ASSERT(form == DW_FORM_data1 || form == DW_FORM_data2 || form == DW_FORM_data4 || form == DW_FORM_data8 || form == DW_FORM_udata);
#endif
  constant_t result;
  switch(form)
  {
    case DW_FORM_data1:
    {
      uint8_t data;
      dwarf_read(debug_info_ptr, data);
      result = data;
      break;
    }
    case DW_FORM_data2:
    {
      uint16_t data;
      dwarf_read(debug_info_ptr, data);
      result = data;
      break;
    }
    case DW_FORM_data4:
    {
      uint32_t data;
      dwarf_read(debug_info_ptr, data);
      result = data;
      break;
    }
    case DW_FORM_udata:
    {
      uLEB128_t data;
      dwarf_read(debug_info_ptr, data);
      result = data;
      break;
    }
    case DW_FORM_data8:
#if defined(__x86_64__) || defined(__sparc64) || defined(__ia64__)
    {
      uint64_t data;
      dwarf_read(debug_info_ptr, data);
      result = data;
      break;
    }
#endif
    // case DW_FORM_sdata:
    default:	// Using default in order to avoid a warning 'result might be used uninitialized'.
      DoutFatal(dc::fatal, "read_constant() cannot handle this FORM");
  }
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  DoutDwarf(dc::finish, result);
#endif
  return result;
}

inline flag_t
read_flag(unsigned char const*& debug_info_ptr, uLEB128_t const DEBUGDWARF_OPT(form))
{
#if DEBUGDWARF
  LIBCWD_ASSERT(form == DW_FORM_flag);
  LIBCWD_TSD_DECLARATION;		// Needed for the DoutDwarf below.
#endif
  uint8_t result;
  dwarf_read(debug_info_ptr, result);
  DoutDwarf(dc::finish, '(' << print_DW_FORM_name(form) << ") \"" << result << '"');
  return result;
}

inline reference_t
read_reference(unsigned char const*& debug_info_ptr, uLEB128_t const form,
               unsigned char const* debug_info_root, unsigned char const* debug_info_start)
{
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT(form == DW_FORM_ref1 || form == DW_FORM_ref2 || form == DW_FORM_ref4 ||
                form == DW_FORM_ref_udata || form == DW_FORM_ref_addr);
  size_t compilation_unit_offset = debug_info_root - debug_info_start;
#endif
  switch(form)
  {
    case DW_FORM_ref1:
    {
      uint8_t offset;
      dwarf_read(debug_info_ptr, offset);
      DoutDwarf(dc::finish, '<' << std::hex << compilation_unit_offset + offset << '>');
      return debug_info_root + offset;
    }
    case DW_FORM_ref2:
    {
      uint16_t offset;
      dwarf_read(debug_info_ptr, offset);
      DoutDwarf(dc::finish, '<' << std::hex << compilation_unit_offset + offset << '>');
      return debug_info_root + offset;
    }
    case DW_FORM_ref4:
    {
      uint32_t offset;
      dwarf_read(debug_info_ptr, offset);
      DoutDwarf(dc::finish, '<' << std::hex << compilation_unit_offset + offset << '>');
      return debug_info_root + offset;
    }
    case DW_FORM_ref_udata:
    {
      uLEB128_t offset;
      dwarf_read(debug_info_ptr, offset);
      DoutDwarf(dc::finish, '<' << std::hex << compilation_unit_offset + offset << '>');
      return debug_info_root + offset;
    }
    case DW_FORM_ref_addr:
    {
      address_t offset;
      dwarf_read(debug_info_ptr, offset);
      DoutDwarf(dc::finish, '<' << std::hex << offset << '>');
      return debug_info_start + offset;
    }
#if 0
    case DW_FORM_ref8:
      DoutFatal(dc::fatal, "read_reference() cannot handle DW_FORM_ref8");
#endif
  }
  abort();
}

inline string_t
read_string(unsigned char const*& debug_info_ptr, uLEB128_t const form, unsigned char const* debug_str)
{
  string_t result;
  if (form == DW_FORM_string)
  {
    result = reinterpret_cast<string_t>(debug_info_ptr);
    eat_string(debug_info_ptr);
  }
  else
#if DEBUGDWARF
  if (form == DW_FORM_strp)
#endif
  {
    result = reinterpret_cast<string_t>(&debug_str[reinterpret_cast_align<uint32_t>(debug_info_ptr)]);
    debug_info_ptr += 4;
  }
#if DEBUGDWARF
  else
    DoutFatal(dc::core, "Calling read_string with form != DW_FORM_string && form != DW_FORM_strp");

  LIBCWD_TSD_DECLARATION;
  DoutDwarf(dc::finish, '(' << print_DW_FORM_name(form) << ") \"" << result << '"');
#endif
  return result;
}

size_t read_length(unsigned char const*& in, size_t& length)
{
  uint32_t len32;
  dwarf_read(in, len32);
  length = len32;
#if defined(__x86_64__) || defined(__sparc64) || defined(__ia64__)
  if (len32 == 0xffffffff)
  {
    uint64_t len64;
    dwarf_read(in, len64);
    length = len64;
    return 12;
  }
#endif
  LIBCWD_ASSERT(len32 < 0xfffffff0);
  return 4;
}

lineptr_t
read_lineptr(unsigned char const*& debug_info_ptr, uLEB128_t const form, lineptr_t debug_line)
{
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT(form == DW_FORM_data4 || form == DW_FORM_data8 || form == DW_FORM_sec_offset);
#endif
  size_t line_offset = 0;
  if (form == DW_FORM_data4)
  {
    uint32_t value;
    dwarf_read(debug_info_ptr, value);
    line_offset = value;
  }
#if defined(__x86_64__) || defined(__sparc64) || defined(__ia64__)
  else if (form == DW_FORM_data8)
  {
    uint64_t value;
    dwarf_read(debug_info_ptr, value);
    line_offset = value;
  }
#endif
  else
  {
    read_length(debug_info_ptr, line_offset);
  }
  DoutDwarf(dc::finish, "0x" << std::hex << line_offset);
  return debug_line + line_offset;
}

// Inline encodings
//static unsigned int const DW_INL_not_inlined		= 0;
//static unsigned int const DW_INL_inlined		= 1;
//static unsigned int const DW_INL_declared_not_inlined	= 2;
//static unsigned int const DW_INL_declared_inlined	= 3;

#if DEBUGDWARF
char const* print_DW_INL_name(constant_t inline_encoding)
{
  switch(inline_encoding)
  {
    case DW_INL_not_inlined: return "DW_INL_not_inlined";
    case DW_INL_inlined: return "DW_INL_inlined";
    case DW_INL_declared_not_inlined: return "DW_INL_declared_not_inlined";
    case DW_INL_declared_inlined: return "DW_INL_declared_inlined";
  }
  abort();
}
#endif

//==========================================================================================================================================
// Because the functions in this compilation unit can be called from malloc(), we need to use
// the alternative allocators.  Moreover, `internal' should already be set everywhere and
// the object_files_instance mutex should be write locked, so we will use `object_files_string'.
// Also, define a replacement type for set<string> here.

#if CWDEBUG_ALLOC
typedef std::basic_string<char, std::char_traits<char>, _private_::object_files_allocator> object_files_string;
typedef std::set<object_files_string, std::less<object_files_string>, _private_::object_files_allocator::rebind<object_files_string>::other> object_files_string_set_ct;
#else
typedef std::string object_files_string;
typedef std::set<object_files_string, std::less<object_files_string> > object_files_string_set_ct;
#endif

//==========================================================================================================================================
// struct location_ct
//
// Internal representation for locations.
//

struct range_st {
  Elfxx_Addr start;
  size_t size;
};

struct location_st {
  object_files_string_set_ct::iterator M_stabs_symbol_funcname_iter;		// Only valid when M_stabs_symbol is set.
  object_files_string_set_ct::iterator M_source_iter;
  Elfxx_Half M_line;
  bool M_stabs_symbol;

  location_st() { }

  location_st(location_st const& loc) :
      M_source_iter(loc.M_source_iter),
      M_line(loc.M_line),
      M_stabs_symbol(loc.M_stabs_symbol)
      {
        if (M_stabs_symbol)
	  M_stabs_symbol_funcname_iter = loc.M_stabs_symbol_funcname_iter;
      }

#if DEBUGSTABS || DEBUGDWARF
  friend std::ostream& operator<<(std::ostream& os, location_st const& loc);
#endif
};

class objfile_ct;

class location_ct : private location_st {
private:
  location_st M_prev_location;
  Elfxx_Addr M_address;
  range_st M_range;
  int M_flags;
  bool M_used;
  objfile_ct* M_object_file;

public:
  location_ct(objfile_ct* object_file) : M_address(0), M_flags(0), M_object_file(object_file)
      { M_prev_location.M_line = (Elfxx_Half)-1; M_line = 0; M_stabs_symbol = false; M_range.start = 0; }

  void invalidate() {
    M_flags = 0;
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
    DoutDwarf(dc::bfd, "--> location invalidated.");
#endif
  }
  void set_line(Elfxx_Half line) {
    if (!(M_flags & 1) || M_line != line)
      M_used = false;
    M_flags |= 1;
    M_line = line;
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
    DoutDwarf(dc::bfd, "--> location.M_line = " << M_line);
#endif
    if (is_valid())
    {
      DoutDwarf(dc::bfd, "--> location now valid.");
      M_store();
    }
  }
  void set_address(Elfxx_Addr address) {
    if (M_address != address)
      M_used = false;
    M_flags |= 2;
    M_address = address;
    if (address == 0x0)		// Happens when the reloc info in the .debug_line
      M_flags &= ~2;		// section refers to non existing labels because
				// the corresponding function instantiation is
				// disregarded (being part of a .gnu.linkonce
				// section that was not used).  We need to ignore
				// this debug info.
				// See the thread of http://gcc.gnu.org/ml/gcc/2003-10/msg00689.html
				// which is about the fact that GNU as version 2.4.90
				// contains a bug that causes the address NOT to be set to 0 :(.
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
#endif
    DoutDwarf(dc::bfd, "--> location.M_address = 0x" << std::hex << address);
    if (is_valid())
    {
      DoutDwarf(dc::bfd, "--> location now valid.");
      M_store();
    }
  }
  void copy()
  {
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
#endif
    // Assume valid. if (is_valid())
    if (M_address)
    {
      DoutDwarf(dc::bfd, "--> location assumed valid.");
      M_flags = 3;
      M_store();
    }
#if DEBUGDWARF
    else
      DoutDwarf(dc::bfd, "--> location not assumed valid (address is 0x0).");
#endif
  }
  void set_source_iter(object_files_string_set_ct::iterator const& iter)
      { M_source_iter = iter; M_used = false; }
  void set_func_iter(object_files_string_set_ct::iterator const& iter)
      { M_stabs_symbol_funcname_iter = iter; M_stabs_symbol = true; }
  // load_stabs doesn't use out M_address.
  bool is_valid_stabs() const { return (M_flags == 1); }
  void increment_line(int increment) {
    if (increment != 0)
      M_used = false;
#if DEBUGDWARF
    bool was_not_valid = !(M_flags & 1);
    LIBCWD_TSD_DECLARATION;
#endif
    M_flags |= 1;
    M_line += increment;
    DoutDwarf(dc::bfd, "--> location.M_line = " << M_line);
    if (is_valid())
    {
#if DEBUGDWARF
      if (was_not_valid)
	DoutDwarf(dc::bfd, "--> location now valid.");
#endif
      M_store();
    }
  }
  void increment_address(unsigned int increment) {
    if (increment > 0)
      M_used = false;
    if (M_address)		// See comment in set_address() above.
      M_address += increment;
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
    DoutDwarf(dc::bfd, "--> location.M_address = 0x" << std::hex << M_address);
#endif
    if (M_address)
      M_flags |= 2;
  }
  void sequence_end() {
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
    DoutDwarf(dc::bfd, "--> Sequence end.");
#endif
    if (is_valid())
    {
      M_line = 0;	// Force storing of range, no range catenation is needed.
      M_store();
    }
    // Invalidate range tracking, we will start all over.
    M_range.start = 0;
  }

  Elfxx_Half get_line() const { LIBCWD_ASSERT( (M_flags & 1) ); return M_line; }
  object_files_string_set_ct::iterator get_source_iter() const { return M_source_iter; }
  Elfxx_Addr get_address() const { return M_address; }

  void stabs_range(range_st const& range) const;

private:
  bool is_valid() const { return (M_flags == 3); }
  void M_store();
};

bool operator==(range_st const&, range_st const&)
{
  DoutFatal(dc::core, "Calling operator==(range_st const& range1, range_st const& range2)");
}

#if DEBUGSTABS || DEBUGDWARF
std::ostream& operator<<(std::ostream& os, range_st const& range)
{
  os << std::hex << range.start << " - " << range.start + range.size;
  return os;
}

std::ostream& operator<<(std::ostream& os, location_st const& loc)
{
  os << (*loc.M_source_iter).data() << ':' << std::dec << loc.M_line;
  if (loc.M_stabs_symbol)
    os << " : \"" << loc.M_stabs_symbol_funcname_iter->data();
  os << "\".";
  return os;
}

std::ostream& operator<<(std::ostream& os, std::pair<range_st const, location_st> const& p)
{
  os << std::hex << p.first.start << " - " << p.first.start + p.first.size << "; " << p.second << '.';
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
  Elfxx_Shdr M_section_header;
public:
  section_ct() { }
  void init(char const* section_header_string_table, Elfxx_Shdr const& section_header);
  Elfxx_Shdr const& section_header() const { return M_section_header; }
};

struct hash_list_st {
  char const* name;
  Elfxx_Addr addr;
  hash_list_st* next;
  bool already_added;
};

//
// class objfile_ct
//
// This object represents an ELF object file.
//

using _private_::compilation_units_vector_ct;

class objfile_ct : public bfd_st {
#if DEBUGSTABS || DEBUGDWARF
  friend void ::debug_load_object_file(char const* filename, bool shared);
#endif
#if CWDEBUG_ALLOC
  typedef std::map<range_st, location_st, compare_range_st, _private_::object_files_allocator::rebind<std::pair<range_st const, location_st> >::other> object_files_range_location_map_ct;
#else
  typedef std::map<range_st, location_st, compare_range_st> object_files_range_location_map_ct;
#endif
private:
  std::ifstream* M_input_stream;
  Elfxx_Ehdr M_header;
  char* M_section_header_string_table;
  section_ct* M_sections;
  char* M_symbol_string_table;
  char* M_dyn_symbol_string_table;
  asymbol_st* M_symbols;
  int M_number_of_symbols;
  Elfxx_Word M_symbol_table_type;
  object_files_string_set_ct M_function_names;
  object_files_string_set_ct M_source_files;
  object_files_range_location_map_ct M_ranges;
  bool volatile M_debug_info_loaded;
  bool M_brac_relative_to_fun;
#if LIBCWD_THREAD_SAFE
  static pthread_t S_thread_inside_find_nearest_line;
#else
  bool M_inside_find_nearest_line;
#endif
#if DEBUGSTABS
  bool M_stabs_debug_info_loaded;
#endif
#if DEBUGDWARF
  bool M_dwarf_debug_info_loaded;
#endif
  Elfxx_Word M_stabs_section_index;
  Elfxx_Word M_stabstr_section_index;
  Elfxx_Word M_dwarf_debug_info_section_index;
  Elfxx_Word M_dwarf_debug_abbrev_section_index;
  Elfxx_Word M_dwarf_debug_line_section_index;
  Elfxx_Word M_dwarf_debug_str_section_index;
  static uint32_t const hash_table_size = 2049;		// Lets use a prime number.
  hash_list_st** M_hash_list;
  hash_list_st* M_hash_list_pool;
  compilation_units_vector_ct M_compilation_units;	// Canonical list of compilation units of this DSO.
public:
  objfile_ct();
  void initialize(char const* file_name);
  ~objfile_ct();
  char const* get_section_header_string_table() const { return M_section_header_string_table; }
  section_ct const& get_section(int index) const { LIBCWD_ASSERT( index < M_header.e_shnum ); return M_sections[index]; }
protected:
  virtual bool check_format() const;
  virtual long get_symtab_upper_bound();
  virtual long canonicalize_symtab(asymbol_st**);
  virtual void find_nearest_line(asymbol_st const*, Elfxx_Addr, char const**, char const**, unsigned int* LIBCWD_COMMA_TSD_PARAM);
  virtual void close();
private:
  void delete_hash_list();
  char* allocate_and_read_section(int i);
  friend class location_ct;	// location_ct::M_store and location_ct::stabs_range need access to `register_range'.
  void register_range(location_st const& location, range_st const& range);
  void load_stabs();
  void load_dwarf();
  uint32_t elf_hash(unsigned char const* name, unsigned char delim) const;
  void eat_form(unsigned char const*& debug_info_ptr, uLEB128_t const& form
                DEBUGDWARF_OPT_COMMA(unsigned char const* debug_str)
                DEBUGDWARF_OPT_COMMA(uint32_t debug_info_offset));
};

#if LIBCWD_THREAD_SAFE
pthread_t objfile_ct::S_thread_inside_find_nearest_line;
#endif

//-------------------------------------------------------------------------------------------------------------------------------------------

void location_ct::M_store()
{
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
#endif
  if (M_used)
  {
    DoutDwarf(dc::bfd, "Skipping M_store: M_used is set.");
    return;
  }
  if (M_line == M_prev_location.M_line && M_prev_location.M_source_iter == M_source_iter)
  {
    DoutDwarf(dc::bfd, "Skipping M_store: location didn't change.");
    return;
  }
#if DEBUGDWARF
  if (M_range.start > M_address)
    core_dump();
#endif
  if (M_range.start == M_address)
    DoutDwarf(dc::bfd, "Skipping M_store: address range is zero.");
  else if (M_range.start)
  {
    DoutDwarf(dc::bfd, "M_store(): Registering new range.");
    M_range.size = M_address - M_range.start;
    M_object_file->register_range(M_prev_location, M_range);
  }
#if DEBUGDWARF
  else
    DoutDwarf(dc::bfd, "M_store(): M_range.start was 0.");
#endif
  M_range.start = M_address;
  M_prev_location.M_stabs_symbol = M_stabs_symbol;
  if (M_stabs_symbol)
    M_prev_location.M_stabs_symbol_funcname_iter = M_stabs_symbol_funcname_iter;
  M_prev_location.M_source_iter = M_source_iter;
  M_prev_location.M_line = M_line;
  M_used = true;
}

inline void location_ct::stabs_range(range_st const& range) const
{
  M_object_file->register_range(*this, range);
}

//-------------------------------------------------------------------------------------------------------------------------------------------
// Implementation

static asection_st const abs_section_c = { 0, "*ABS*", 0 };
asection_st const* const absolute_section_c = &abs_section_c;

static bool check_elf_format(Elfxx_Ehdr const& header)
{
  if (header.e_ident[EI_MAG0] != ELFMAG0 ||
      header.e_ident[EI_MAG1] != ELFMAG1 ||
      header.e_ident[EI_MAG2] != ELFMAG2 ||
      header.e_ident[EI_MAG3] != ELFMAG3)
    Dout(dc::bfd, "Object file must be ELF.");
#if defined(__x86_64__) || defined(__sparc64) || defined(__ia64__)
  else if (header.e_ident[EI_CLASS] != ELFCLASS64)
    Dout(dc::bfd, "Sorry, object file must be ELF64.");
#else
  else if (header.e_ident[EI_CLASS] != ELFCLASS32)
    Dout(dc::bfd, "Sorry, object file must be ELF32.");
#endif
  else if (header.e_ident[EI_DATA] !=
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __LITTLE_ENDIAN
      ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
      ELFDATA2MSB
#else
      ELFDATANONE
#endif
#else // !__BYTE_ORDER
#ifdef WORDS_BIGENDIAN
      ELFDATA2MSB
#else
      ELFDATA2LSB
#endif
#endif // !__BYTE_ORDER
      )
    Dout(dc::bfd, "Object file has non-native data encoding.");
  else if (header.e_ident[EI_VERSION] != EV_CURRENT)
    Dout(dc::warning, "Object file has different version than what libcwd understands.");
  else
    return false; 
  return true;
}

void section_ct::init(char const* section_header_string_table, Elfxx_Shdr const& section_header)
{
  std::memcpy(&M_section_header, &section_header, sizeof(M_section_header));
  static_cast<asection_st*>(this)->M_size = M_section_header.sh_size;	// Used to guess the size of the last symbol in a section.
  // Duplicated values:
  vma = M_section_header.sh_addr;
  name = &section_header_string_table[M_section_header.sh_name];
}

void objfile_ct::close()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( __libcwd_tsd.internal == 1 );
#endif

  _private_::set_alloc_checking_on(LIBCWD_TSD);

  Debug( libcw_do.off() );
  delete M_input_stream;
  Debug( libcw_do.on() );

  LIBCWD_DEFER_CLEANUP_PUSH(&_private_::rwlock_tct<object_files_instance>::cleanup, NULL); // The delete calls close().
  BFD_ACQUIRE_WRITE_LOCK;
  _private_::set_alloc_checking_off(LIBCWD_TSD);
  delete this;
  _private_::set_alloc_checking_on(LIBCWD_TSD);
  BFD_RELEASE_WRITE_LOCK;
  LIBCWD_CLEANUP_POP_RESTORE(false);

  _private_::set_alloc_checking_off(LIBCWD_TSD);
}

objfile_ct::~objfile_ct()
{
  delete_hash_list();
  delete [] M_section_header_string_table;
  delete [] M_sections;
  delete [] M_symbol_string_table;
  delete [] M_dyn_symbol_string_table;
  delete [] M_symbols;
}

long objfile_ct::get_symtab_upper_bound()
{
  return M_number_of_symbols * sizeof(asymbol_st*);
}

bool objfile_ct::check_format() const
{
  return check_elf_format(M_header);
}

uint32_t objfile_ct::elf_hash(unsigned char const* name, unsigned char delim) const
{
  uint32_t h = 0;
  uint32_t g;
  while (*name != delim)
  {
    h = (h << 4) + *name++;
    if ((g = (h & 0xf0000000)))
      h ^= g >> 24;
    h &= ~g;
  }
  return (h % hash_table_size);
}

long objfile_ct::canonicalize_symtab(asymbol_st** symbol_table)
{
#if DEBUGELFXX
  LIBCWD_TSD_DECLARATION;
#endif
  M_symbols = new asymbol_st[M_number_of_symbols];
  M_hash_list = new hash_list_st* [hash_table_size];
  M_hash_list_pool = NULL;
  std::memset(M_hash_list, 0, hash_table_size * sizeof(hash_list_st*));
  asymbol_st* new_symbol = M_symbols;
  int table_entries = 0;
  for(int i = 0; i < M_header.e_shnum; ++i)
  {
    if ((M_sections[i].section_header().sh_type == M_symbol_table_type)
        && M_sections[i].section_header().sh_size > 0)
    {
      int number_of_symbols = M_sections[i].section_header().sh_size / sizeof(Elfxx_Sym);
      DoutElfxx(dc::bfd, "Found symbol table " << M_sections[i].name << " with " << number_of_symbols << " symbols.");
      Elfxx_Sym* symbols = (Elfxx_Sym*)allocate_and_read_section(i);
      M_hash_list_pool = (hash_list_st*)malloc(sizeof(hash_list_st) * number_of_symbols);
      hash_list_st* hash_list_pool_next = M_hash_list_pool;
      for(int s = 0; s < number_of_symbols; ++s)
      {
	Elfxx_Sym& symbol(symbols[s]);
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
	  new_symbol->section = absolute_section_c;
          M_s_end_offset = new_symbol->value = symbol.st_value;
	}
        else if (symbol.st_shndx >= SHN_LORESERVE || symbol.st_shndx == SHN_UNDEF)
	  continue;							// Skip Special Sections and Undefined Symbols.
	else if (ELFXX_ST_TYPE(symbol.st_info) >= STT_FILE)
	  continue;							// Skip STT_FILE, STT_COMMON and STT_TLS symbols.
	else
	{
	  new_symbol->section = &M_sections[symbol.st_shndx];
	  new_symbol->value = symbol.st_value - new_symbol->section->vma;	// Is not an absolute value: make value relative

	  if (new_symbol->section->name[1] == 'b' && new_symbol->section->name[0] == '.' && new_symbol->section->name[2] == 's' &&
	      new_symbol->section->name[3] == 's' && new_symbol->section->name[4] == 0)
	  {
	    // On solaris 2.8 _end is not absolute but in .bss.
	    // This is now also the case on linux.
	    if (new_symbol->name[1] == 'e' && new_symbol->name[0] == '_' && new_symbol->name[2] == 'n' && new_symbol->name[3] == 'd' && new_symbol->name[4] == 0)
	      M_s_end_offset = symbol.st_value;
	  }
	  									// to start of section.
	  DoutElfxx(dc::bfd, "Symbol \"" << new_symbol->name << "\" in section \"" << new_symbol->section->name << "\" at 0x" <<
	      std::hex << (new_symbol->section->vma + new_symbol->value) << ".");
        }
	new_symbol->bfd_ptr = this;
	new_symbol->size = symbol.st_size;
	new_symbol->flags = 0;
	switch(ELFXX_ST_BIND(symbol.st_info))
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
        switch(ELFXX_ST_TYPE(symbol.st_info))
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
	}
	// Filter out symbols that could endanger determining the size of a symbol.
	if ((new_symbol->flags & (cwbfd::BSF_GLOBAL|cwbfd::BSF_FUNCTION|cwbfd::BSF_OBJECT)) == 0)
          continue;
	uint32_t hash = elf_hash(reinterpret_cast<unsigned char const*>(new_symbol->name), (unsigned char)0);
	hash_list_st** p = &M_hash_list[hash];
	while(*p)
	  p = &(*p)->next;
	*p = hash_list_pool_next++;
        (*p)->next = NULL;
        (*p)->name = new_symbol->name;
        (*p)->addr = symbol.st_value;
	(*p)->already_added = false;
	symbol_table[table_entries++] = new_symbol++;
      }
      // The assignment is needed in case hash_list_pool_next == M_hash_list_pool.
      M_hash_list_pool = (hash_list_st*)realloc(M_hash_list_pool,
                                                (hash_list_pool_next - M_hash_list_pool) * sizeof(hash_list_st));
      delete [] symbols;
      break;							// There *should* only be one symbol table section.
    }
  }
  LIBCWD_ASSERT( M_number_of_symbols >= table_entries );
  M_number_of_symbols = table_entries;
  return M_number_of_symbols;
}

struct attr_st {
  union {
    unsigned long attr;		// Really a uLEB128_t, but we can't use that in a union.
    unsigned char count;
  };
  uLEB128_t form;
};

struct abbrev_st {
  uLEB128_t code;
  uLEB128_t tag;
  attr_st* attributes;
  unsigned short attributes_size;
  unsigned short attributes_capacity;
  unsigned int fixed_size;
  bool starts_with_string;
  bool has_children;
  abbrev_st(abbrev_st const& abbrev);
  abbrev_st() : attributes(NULL), attributes_size(0), attributes_capacity(0) { }
  ~abbrev_st() { if (attributes && --attributes[attributes_capacity].count == 0) free(attributes); }
};

abbrev_st::abbrev_st(abbrev_st const& abbrev)
{
  if (&abbrev != this)
  {
    std::memcpy(this, &abbrev, sizeof(abbrev_st));
    if (attributes)
      ++attributes[attributes_capacity].count;
  }
}

struct file_name_st {
  char const* name;
  uLEB128_t directory_index;
  uLEB128_t time_of_last_modification;
  uLEB128_t length_in_bytes_of_the_file;
};

static object_files_string catenate_path(object_files_string const& dir, char const* subpath)
{
  object_files_string res;
  int count = 0;
  while (subpath[0] == '.' && subpath[1] == '.' && subpath[2] == '/')
  {
    ++count;
    subpath += 3;
  }
  if (count > 0)
  {
    size_t pos = dir.size() - 1;
    for(; count > 0; --count)
      pos = dir.find_last_of('/', pos - 1);
    res.assign(dir, 0, pos + 1);
  }
  else
    res.assign(dir);
  res.append(subpath);
  return res;
}

void objfile_ct::delete_hash_list()
{
  if (M_hash_list)
  {
    if (M_hash_list_pool)
    {
      free(M_hash_list_pool);
      M_hash_list_pool = NULL;
    }
    delete [] M_hash_list;
    M_hash_list = NULL;
  }
}

void objfile_ct::eat_form(unsigned char const*& debug_info_ptr, uLEB128_t const& form
                          DEBUGDWARF_OPT_COMMA(unsigned char const* debug_str)
			  DEBUGDWARF_OPT_COMMA(uint32_t debug_info_offset))
{
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  DoutDwarf(dc::continued, '(' << print_DW_FORM_name(form) << ") ");
#endif
  switch(form)
  {
    case DW_FORM_data1:
    case DW_FORM_ref1:
    case DW_FORM_flag:
      DoutDwarf(dc::finish, (int)*reinterpret_cast<uint8_t const*>(debug_info_ptr));
      debug_info_ptr += 1;
      break;
    case DW_FORM_data2:
    case DW_FORM_ref2:
      DoutDwarf(dc::finish, reinterpret_cast_align<uint16_t>(debug_info_ptr));
      debug_info_ptr += 2;
      break;
    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_strp:
    case DW_FORM_sec_offset:
#if DEBUGDWARF
      if (form == DW_FORM_data4)
	DoutDwarf(dc::finish, reinterpret_cast_align<uint32_t>(debug_info_ptr));
      else if (form == DW_FORM_strp)
      {
	unsigned int pos = reinterpret_cast_align<uint32_t>(debug_info_ptr);
	DoutDwarf(dc::finish, pos << " (\"" << &debug_str[pos] << "\")");
      }
      else if (form == DW_FORM_sec_offset)
      {
	unsigned int pos = reinterpret_cast_align<uint32_t>(debug_info_ptr);
	DoutDwarf(dc::finish, pos);
      }
      else
	DoutDwarf(dc::finish, '<' << std::hex <<
	    reinterpret_cast_align<uint32_t>(debug_info_ptr) + debug_info_offset << '>');
#endif
      debug_info_ptr += 4;
      break;
    case DW_FORM_data8:
    case DW_FORM_ref8:
    case DW_FORM_ref_sig8:
#if DEBUGDWARF && (defined(__x86_64__) || defined(__sparc64) || defined(__ia64__))
      if (form == DW_FORM_data8 || form == DW_FORM_ref_sig8)
	DoutDwarf(dc::finish, reinterpret_cast_align<uint64_t>(debug_info_ptr));
      else
	DoutDwarf(dc::finish, '<' << std::hex <<
	    reinterpret_cast_align<uint64_t>(debug_info_ptr) + debug_info_offset << '>');
#endif
      debug_info_ptr += 8;
      break;
    case DW_FORM_indirect:
    {
      uLEB128_t tmpform = form;
      DoutDwarf(dc::continued, "-> ");
      dwarf_read(debug_info_ptr, tmpform);
      eat_form(debug_info_ptr, tmpform DEBUGDWARF_OPT_COMMA(debug_str) DEBUGDWARF_OPT_COMMA(debug_info_offset));
      break;
    }
    case DW_FORM_string:
    {
      DoutDwarf(dc::finish, "\"" << reinterpret_cast<char const*>(debug_info_ptr) << '"');
      eat_string(debug_info_ptr);
      break;
    }
    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_ref_udata:
    {
      uLEB128_t data;
      dwarf_read(debug_info_ptr, data);
      DoutDwarf(dc::finish, data);
      break;
    }
    case DW_FORM_ref_addr:
    case DW_FORM_addr:
      DoutDwarf(dc::finish, reinterpret_cast_align<void*>(debug_info_ptr));
      debug_info_ptr += address_size;
      break;
    case DW_FORM_block1:
    {
      uint8_t length;
      dwarf_read(debug_info_ptr, length);
      DoutDwarf(dc::finish, "[" << (unsigned int)length << " bytes]");
      debug_info_ptr += length;
      break;
    }
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
    case DW_FORM_exprloc:
    {
      uLEB128_t length;
      dwarf_read(debug_info_ptr, length);
      DoutDwarf(dc::finish, "[" << length << " bytes]");
      debug_info_ptr += length;
      break;
    }
    case DW_FORM_flag_present:
      DoutDwarf(dc::finish, "<present>");
      break;
  }
}

void objfile_ct::load_dwarf()
{
#if DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  size_t total_length;
#endif

#if CWDEBUG_ALLOC
#if !DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
#endif
  int saved_internal = __libcwd_tsd.internal;
  __libcwd_tsd.internal = false;
#endif
  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << this->filename_str << "... ");
#if CWDEBUG_ALLOC
  __libcwd_tsd.internal = saved_internal;
#endif

#if DEBUGDWARF
  // Not loaded already.
  LIBCWD_ASSERT( !M_dwarf_debug_info_loaded );
  M_dwarf_debug_info_loaded = true;
  // Don't have fixed entry sizes.
  LIBCWD_ASSERT( M_sections[M_dwarf_debug_line_section_index].section_header().sh_entsize == 0 );
  LIBCWD_ASSERT( M_sections[M_dwarf_debug_info_section_index].section_header().sh_entsize == 0 );
  LIBCWD_ASSERT( M_sections[M_dwarf_debug_abbrev_section_index].section_header().sh_entsize == 0 );
  // gcc 3.1 sets sh_entsize to 1, previous versions set it to 0.
  LIBCWD_ASSERT( M_dwarf_debug_str_section_index == 0 || M_sections[M_dwarf_debug_str_section_index].section_header().sh_entsize <= 1 );
  // Initialization of debug variable.
  total_length = 0;
#endif

  // Start of .debug_abbrev section.
  unsigned char* debug_abbrev = (unsigned char*)allocate_and_read_section(M_dwarf_debug_abbrev_section_index);
  // Start of .debug_info section.
  unsigned char* debug_info = (unsigned char*)allocate_and_read_section(M_dwarf_debug_info_section_index);
  unsigned char* const debug_info_start = debug_info;
  unsigned char* debug_info_end = debug_info + M_sections[M_dwarf_debug_info_section_index].section_header().sh_size;
  // Start of .debug_line section.
  lineptr_t debug_line = reinterpret_cast<lineptr_t>(allocate_and_read_section(M_dwarf_debug_line_section_index));
  lineptr_t debug_line_end = debug_line + M_sections[M_dwarf_debug_line_section_index].section_header().sh_size;
  lineptr_t debug_line_ptr = NULL;	// Initialization not used; only here to avoid compiler warning.
  // Start of .debug_str section.
  // Needed for DW_FORM_strp.
  unsigned char* debug_str = (unsigned char*)allocate_and_read_section(M_dwarf_debug_str_section_index);

  // Run over all compilation units.
  for (unsigned char const* debug_info_ptr = debug_info; debug_info_ptr < debug_info_end;)
  {
    unsigned char const* const debug_info_root = debug_info_ptr;
    size_t length;
#if DEBUGDWARF
    size_t length_size =
#endif
        read_length(debug_info_ptr, length);
#if DEBUGDWARF
    LIBCWD_TSD_DECLARATION;
    if (doutdwarfon)
    {
      _private_::set_alloc_checking_on(LIBCWD_TSD);	// Needed for Dout().
      Dout(dc::bfd, "debug_info_ptr = " << (void*)debug_info_ptr << "; debug_info_end = " << (void*)debug_info_end);
      Dout(dc::bfd, "length = " << length << " (0x" << std::hex << length << std::dec << ')');
      total_length += length + length_size;
      Dout(dc::bfd, "total length = " << total_length << " (of " <<
	  M_sections[M_dwarf_debug_info_section_index].section_header().sh_size << ").");
      _private_::set_alloc_checking_off(LIBCWD_TSD);
    }
    LIBCWD_ASSERT( total_length <= M_sections[M_dwarf_debug_info_section_index].section_header().sh_size );
#endif
    uint16_t version;
    dwarf_read(debug_info_ptr, version);
    if ( version >= 2 && version <= 4 )	// DWARF version 2 (and 3 and 4)
    {
      DoutDwarf(dc::bfd, "version = " << version);
      uint32_t abbrev_offset;
      dwarf_read(debug_info_ptr, abbrev_offset);
      unsigned char const* debug_abbrev_ptr = debug_abbrev + abbrev_offset;
      DoutDwarf(dc::bfd, "abbrev_offset = 0x" << std::hex << abbrev_offset);
      dwarf_read(debug_info_ptr, address_size);
      LIBCWD_ASSERT( address_size == sizeof(void*) );

      unsigned int expected_code = 1;
#if CWDEBUG_ALLOC
      std::vector<abbrev_st, _private_::object_files_allocator::rebind<abbrev_st>::other> abbrev_entries(256);
#else
      std::vector<abbrev_st> abbrev_entries(256);
#endif
      while(true)
      {
	if (expected_code >= abbrev_entries.size())
	  abbrev_entries.resize(abbrev_entries.size() + 256);
	abbrev_st& abbrev(abbrev_entries[expected_code]);

	dwarf_read(debug_abbrev_ptr, abbrev.code);
	if (abbrev.code == 0)
	  break;

	dwarf_read(debug_abbrev_ptr, abbrev.tag);
	unsigned char children;
	dwarf_read(debug_abbrev_ptr, children);
	abbrev.has_children = (children == DW_CHILDREN_yes);
	bool has_fixed_size = !abbrev.has_children;
	unsigned int fixed_size = 0;
	bool first_time = true;
	abbrev.starts_with_string = false;

	DoutDwarf(dc::bfd, "Abbr. " << abbrev.code << " \t" << print_DW_TAG_name(abbrev.tag) <<
	    " \t[" << (children ? "has" : "no") << " children]");

	LIBCWD_ASSERT( abbrev.code == expected_code );
	++expected_code;

        while(true)
	{
	  if (abbrev.attributes_size == abbrev.attributes_capacity)
	  {
	    abbrev.attributes_capacity += 32;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
	    abbrev.attributes = (attr_st*)realloc(abbrev.attributes, (abbrev.attributes_capacity + 1) * sizeof(attr_st));
#pragma GCC diagnostic pop
	    abbrev.attributes[abbrev.attributes_capacity].count = 1;
	  }
	  uLEB128_t& attr(*reinterpret_cast<uLEB128_t*>(&abbrev.attributes[abbrev.attributes_size].attr));
	  uLEB128_t& form(abbrev.attributes[abbrev.attributes_size].form);
	  dwarf_read(debug_abbrev_ptr, attr);
	  dwarf_read(debug_abbrev_ptr, form);
	  DoutDwarf(dc::bfd, "  Attribute/Form: " << print_DW_AT_name(attr) << ", " << print_DW_FORM_name(form));
	  if (attr == 0 && form == 0)
	    break;
	  if (has_fixed_size)
	  {
	    switch(form)
	    {
	      case DW_FORM_data1:
	      case DW_FORM_ref1:
	      case DW_FORM_flag:
	        fixed_size += 1;
		break;
	      case DW_FORM_data2:
	      case DW_FORM_ref2:
	        fixed_size += 2;
		break;
	      case DW_FORM_data4:
	      case DW_FORM_ref4:
	      case DW_FORM_strp:
	        fixed_size += 4;
		break;
	      case DW_FORM_data8:
	      case DW_FORM_ref8:
	        fixed_size += 8;
		break;
	      case DW_FORM_ref_addr:
	      case DW_FORM_addr:
	        fixed_size += address_size;
		break;
	      default:
	        if (first_time)
		{
		  abbrev.starts_with_string = (form == DW_FORM_string);
		  if (abbrev.starts_with_string)
		    break;
		}
	        has_fixed_size = false;
		break;
            }
	  }
	  first_time = false;
	  ++abbrev.attributes_size;
	}
	abbrev.fixed_size = has_fixed_size ? fixed_size : 0;
	abbrev.attributes_capacity = abbrev.attributes_size;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
	abbrev.attributes = (attr_st*)realloc(abbrev.attributes, (abbrev.attributes_capacity + 1) * sizeof(attr_st));
#pragma GCC diagnostic pop
	abbrev.attributes[abbrev.attributes_capacity].count = 1;
      }

      using _private_::compilation_unit_ct;
      compilation_unit_ct* current_compilation_unit = 0;
      int abstract_instances = 0;
      int abstract_instances_referenced = 0;
      int level = 0;
      unsigned char const* debug_info_ptr_end = debug_info_ptr + length - sizeof(version) - sizeof(abbrev_offset) - sizeof(address_size);
      while(debug_info_ptr < debug_info_ptr_end)
      {
#if DEBUGDWARF
        unsigned int reference = debug_info_ptr - debug_info_start;
#endif
	uLEB128_t code;
	dwarf_read(debug_info_ptr, code);
	if (code == 0)
	{
	  DoutDwarf(dc::bfd, '<' << std::hex << reference << "> code: 0");
          if (DEBUGDWARF)
	    Debug( libcw_do.marker().assign(libcw_do.marker().c_str(), libcw_do.marker().size() - 1) );
	  if (--level <= 0)
	  {
	    if (DEBUGDWARF)
	      LIBCWD_ASSERT( level == 0 );
	    break;
          }
	  continue;
	}
	LIBCWD_ASSERT( code < abbrev_entries.size() );
	abbrev_st& abbrev(abbrev_entries[code]);
	DoutDwarf(dc::bfd, '<' << std::hex << reference <<
	    "> Abbrev Number: " << std::dec << code << " (" << print_DW_TAG_name(abbrev.tag) << ')');
        if (DEBUGDWARF)
	  Debug(libcw_do.inc_indent(4));

	attr_st* attr = abbrev.attributes;

	if (abbrev.tag == DW_TAG_compile_unit)
	{
	  object_files_string default_dir;
	  object_files_string cur_dir;
	  object_files_string default_source;
	  address_t low_pc = 0;
	  address_t high_pc = 0;
	  bool found_stmt_list = false;

	  M_compilation_units.push_back(compilation_unit_ct());
	  current_compilation_unit = &M_compilation_units.back();

	  for (int i = 0; i < abbrev.attributes_size; ++i, ++attr)
	  {
	    uLEB128_t form = attr->form;
	    DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr->attr) << ' ');
	    if (attr->attr == DW_AT_stmt_list)
	    {
	      debug_line_ptr = read_lineptr(debug_info_ptr, form, debug_line);
	      found_stmt_list = true;
	      continue;
	    }
	    else if (attr->attr == DW_AT_name || attr->attr == DW_AT_comp_dir)
	    {
	      string_t str = read_string(debug_info_ptr, attr->form, debug_str);
	      if (attr->attr == DW_AT_comp_dir)
	      {
		default_dir.assign(str);
		default_dir += '/';
                current_compilation_unit->set_compilation_directory(default_dir);
	      }
	      else
	      {
		if (str[0] == '.' && str[1] == '/')
		  str += 2;
		default_source.assign(str);
                current_compilation_unit->set_source_file(default_source);
		default_source += '\0';
	      }
	      continue;
	    }
	    else if (attr->attr == DW_AT_high_pc || attr->attr == DW_AT_low_pc)
	    {
	      address_t address;
	      if (attr->attr == DW_AT_high_pc && form != DW_FORM_addr)	// Should be class constant, ie DW_FORM_data8
	      {
		constant_t size = read_constant(debug_info_ptr, form);
		address = size + reinterpret_cast<Elfxx_Addr>(current_compilation_unit->get_lowpc());
	      }
	      else
		address = read_address(debug_info_ptr, form);
	      if (attr->attr == DW_AT_high_pc)
	      {
	        high_pc = address;
		current_compilation_unit->set_highpc(high_pc);
	      }
	      else
	      {
		low_pc = address;
		current_compilation_unit->set_lowpc(low_pc);
	      }
	      continue;
	    }
	    eat_form(debug_info_ptr, form DEBUGDWARF_OPT_COMMA(debug_str)
	        DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
	  }

	  if (DEBUGDWARF)
	    Debug(libcw_do.dec_indent(4));
	  if (abbrev.has_children)
	  {
	    ++level;
	    if (DEBUGDWARF)
	      Debug( libcw_do.marker().append("|", 1) );
	  }

	  if (found_stmt_list && debug_line_ptr < debug_line_end && !(low_pc && high_pc && low_pc == high_pc))
	  {
	    // ===========================================================================================================================17"
	    // State machine.
	    // See paragraph 6.2 of "DWARF Debugging Information Format" document.

	    uLEB128_t file;		// An unsigned integer indicating the identity of the source file corresponding to a machine
					// instruction.
	    uLEB128_t column;		// An unsigned integer indicating a column number within a source line.  Columns are numbered
					// beginning at 1. The value 0 is reserved to indicate that a statement begins at the left
					// edge of the line.
	    bool is_stmt;		// A boolean indicating that the current instruction is the beginning of a statement.
	    bool basic_block __attribute__((unused));		// A boolean indicating that the current instruction is the beginning of a basic block.
	    bool end_sequence;		// A boolean indicating that the current address is that of the first byte after the end of
					// a sequence of target machine instructions.
	    uint32_t total_length;	// The size in bytes of the statement information for this compilation unit (not including
					// the total_length field itself).
	    uint16_t version;		// Version identifier for the statement information format.
	    uint32_t prologue_length;	// The number of bytes following the prologue_length field to the beginning of the first
					// byte of the statement program itself.
	    unsigned char minimum_instruction_length;	// The size in bytes of the smallest target machine instruction.  Statement
					// program opcodes that alter the address register first multiply their operands by this value.
	    unsigned char default_is_stmt;// The initial value of the is_stmt register.
	    signed char line_base;	// This parameter affects the meaning of the special opcodes.
	    unsigned char line_range;	// This parameter affects the meaning of the special opcodes.
	    unsigned char opcode_base;	// The number assigned to the first special opcode.
	    unsigned char const* standard_opcode_lengths;
#if CWDEBUG_ALLOC
	    std::vector<char const*, _private_::object_files_allocator::rebind<char const*>::other> include_directories;
	    std::vector<file_name_st, _private_::object_files_allocator::rebind<file_name_st>::other> file_names;
#else
	    std::vector<char const*> include_directories;
	    std::vector<file_name_st> file_names;
#endif
	    dwarf_read(debug_line_ptr, total_length);
	    LIBCWD_ASSERT(total_length < 0xfffffff0);
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
	      while(*debug_line_ptr++) ;
	    }
	    ++debug_line_ptr;
	    while(true)
	    {
	      if (DEBUGDWARF)
		LIBCWD_ASSERT( debug_line_ptr < statement_program_start );
	      file_name_st file_name;
	      file_name.name = reinterpret_cast<char const*>(debug_line_ptr);
	      if (!*debug_line_ptr++)
		break;
	      while(*debug_line_ptr++) ;
	      dwarf_read(debug_line_ptr, file_name.directory_index);
	      dwarf_read(debug_line_ptr, file_name.time_of_last_modification);
	      dwarf_read(debug_line_ptr, file_name.length_in_bytes_of_the_file);
	      DoutDwarf(dc::bfd, "File name: " << file_name.name);
	      file_names.push_back(file_name);
	    }
	    LIBCWD_ASSERT( debug_line_ptr == statement_program_start );

	    object_files_string cur_dir;
	    object_files_string cur_source;
	    location_ct location(this);

	    while( debug_line_ptr < debug_line_ptr_end )
	    {
	      file = 0;			// One less than the `file' mentioned in the documentation.
	      column = 0;
	      is_stmt = default_is_stmt;
	      basic_block = false;
	      end_sequence = false;
	      if (default_source[0] == '/')
	        cur_dir.erase();
              else
		cur_dir = default_dir;
	      cur_source = catenate_path(cur_dir, default_source.data());
	      if (default_dir[0] == '.' && default_dir[1] == '.' && default_dir[2] == '/')
	      {
		cur_source.assign(cur_dir, 0, cur_dir.find_last_of('/', cur_dir.size() - 2) + 1);
		cur_source.append(default_source, 3, object_files_string::npos);
	      }
	      else
		cur_source = cur_dir + default_source;

	      location.invalidate();
	      location.set_source_iter(M_source_files.insert(cur_source).first);
	      location.set_line(1);

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
		      LIBCWD_ASSERT( size > 0 );
		      uLEB128_t extended_opcode;
		      dwarf_read(debug_line_ptr, extended_opcode);
		      LIBCWD_ASSERT( extended_opcode < 0x80 );		// Then it's size is one:
		      --size;
		      switch(extended_opcode)
		      {
			case DW_LNE_end_sequence:
			  LIBCWD_ASSERT( size == 0 );
			  end_sequence = true;
			  DoutDwarf(dc::bfd, "DW_LNE_end_sequence: Address: 0x" << std::hex << location.get_address());
			  location.sequence_end();
			  break;
			case DW_LNE_set_address:
			{
			  Elfxx_Addr address;
			  LIBCWD_ASSERT( size == sizeof(address) );
			  dwarf_read(debug_line_ptr, address);
			  DoutDwarf(dc::bfd, "DW_LNE_set_address: 0x" << std::hex << address);
			  location.set_address(address);
			  break;
			}
			case DW_LNE_define_file:
			{
			  unsigned char const* end = debug_line_ptr + size;
			  file_name_st file_name;
			  file_name.name = reinterpret_cast<char const*>(debug_line_ptr);
			  if (file_name.name[0] == '.' && file_name.name[1] == '/')
			    file_name.name += 2;
			  while(*debug_line_ptr++) ;
			  dwarf_read(debug_line_ptr, file_name.directory_index);
			  dwarf_read(debug_line_ptr, file_name.time_of_last_modification);
			  dwarf_read(debug_line_ptr, file_name.length_in_bytes_of_the_file);
			  LIBCWD_ASSERT( debug_line_ptr == end );
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
		      DoutDwarf(dc::bfd, "DW_LNS_copy");
		      location.copy();
		      basic_block = false;
		      break;
		    case DW_LNS_advance_pc:
		    {
		      uLEB128_t address_increment;
		      dwarf_read(debug_line_ptr, address_increment);
		      DoutDwarf(dc::bfd, "DW_LNS_advance_pc: " << std::hex << address_increment);
		      location.increment_address(minimum_instruction_length * address_increment);
		      break;
		    }
		    case DW_LNS_advance_line:
		    {
		      LEB128_t line_increment;
		      dwarf_read(debug_line_ptr, line_increment);
		      DoutDwarf(dc::bfd, "DW_LNS_advance_line: " << line_increment);
		      location.invalidate();
		      location.increment_line(line_increment);
		      break;
		    }
		    case DW_LNS_set_file:
		    {
		      dwarf_read(debug_line_ptr, file);
		      --file;
		      DoutDwarf(dc::bfd, "DW_LNS_set_file: \"" << file_names[file].name << '"');
		      location.invalidate();
		      if (*file_names[file].name == '/')
			cur_source.assign(file_names[file].name);
		      else
		      {
			if (file_names[file].directory_index == 0)
			  cur_dir = default_dir;
			else
			{
			  if (*include_directories[file_names[file].directory_index - 1] != '/')
			  {
			    cur_dir.assign(current_compilation_unit->get_compilation_directory().data(),
			        current_compilation_unit->get_compilation_directory().length());
			    cur_dir += include_directories[file_names[file].directory_index - 1];
			  }
			  else
			    cur_dir.assign(include_directories[file_names[file].directory_index - 1]);
			  cur_dir += '/';
			}
			cur_source = catenate_path(cur_dir, file_names[file].name);
		      }
		      cur_source += '\0';
		      location.set_source_iter(M_source_files.insert(cur_source).first);
		      break;
		    }
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
		      DoutDwarf(dc::bfd, "DW_LNS_const_add_pc");
		      unsigned int address_increment = (255 - opcode_base) / line_range;
		      location.increment_address(minimum_instruction_length * address_increment);
		      break;
		    }
		    case DW_LNS_fixed_advance_pc:
		    {
		      DoutDwarf(dc::bfd, "DW_LNS_fixed_advance_pc");
		      unsigned short int address_increment;
		      dwarf_read(debug_line_ptr, address_increment);
		      location.increment_address(minimum_instruction_length * address_increment);
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
		  unsigned int address_increment = (opcode - opcode_base) / line_range;
		  DoutDwarf(dc::bfd, "Special opcode.  Address/Line increments: 0x" <<
		      std::hex << address_increment << ", " << std::dec << line_increment);
		  location.increment_address(minimum_instruction_length * address_increment);
		  location.increment_line(line_increment);
		  basic_block = false;
		}
	      }
	    }
	    LIBCWD_ASSERT( debug_line_ptr == debug_line_ptr_end );

	    // End state machine code.
	    // ===========================================================================================================================17"
	  }
	  else
	  {
	    DoutDwarf(dc::bfd, "Skipping compilation unit." << default_dir << default_source.data());
	  }
	  continue;
        }
#if 0
	else if (abbrev.tag == DW_TAG_subprogram)
	{
	  bool saw_low_pc = false;
	  bool saw_high_pc = false;
	  bool saw_declaration = false;
	  bool saw_specification_declaration = false;
	  bool saw_specification = false;
	  bool saw_linkage_name = false;
	  bool saw_inline = false;
	  address_t low_pc = 0;
	  address_t high_pc = 0;
	  string_t linkage_name = 0;
	  string_t name = 0;
	  constant_t inline_attr = 0;
	  for (int i = 0; i < abbrev.attributes_size; ++i, ++attr)
	  {
	    DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr->attr) << ' ');
	    switch(attr->attr)
	    {
	      case DW_AT_inline:
	        LIBCWD_ASSERT(attr->form == DW_FORM_data1);
	        inline_attr = read_constant(debug_info_ptr, attr->form);
		DoutDwarf(dc::bfd, "DW_AT_inline flag is " << inline_attr);
		((uint8_t*)debug_info_ptr)[-1] |= 16;
		DoutDwarf(dc::bfd, "DW_AT_inline flag set to " << (unsigned int)(((uint8_t*)debug_info_ptr)[-1]));
		++abstract_instances;
		DoutDwarf(dc::bfd, "DW_AT_inline count set to " << abstract_instances);
	        saw_inline = true;
	        break;
	      case DW_AT_low_pc:
		low_pc =read_address(debug_info_ptr, attr->form);
		saw_low_pc = true;
	        break;
	      case DW_AT_high_pc:
		high_pc = read_address(debug_info_ptr, attr->form);
		saw_high_pc = true;
	        break;
	      case DW_AT_MIPS_linkage_name:
		saw_linkage_name = true;
		linkage_name = read_string(debug_info_ptr, attr->form, debug_str);
	        break;
	      case DW_AT_abstract_origin:
	      {
	        LIBCWD_ASSERT( attr->form != DW_FORM_ref_addr); // Must be same compilation unit.
		reference_t abstract_debug_info_ptr =
		    read_reference(debug_info_ptr, attr->form, debug_info_root, debug_info_start);
		if (DEBUGDWARF)
		  Debug(libcw_do.inc_indent(4));
		dwarf_read(abstract_debug_info_ptr, code);
		LIBCWD_ASSERT( code < abbrev_entries.size() );
		abbrev_st& abbrev2(abbrev_entries[code]);
		LIBCWD_ASSERT(abbrev2.tag == DW_TAG_subprogram);
		bool saw_inline2 = false;
		attr_st* attr2 = abbrev2.attributes;
		for (int i = 0; i < abbrev2.attributes_size; ++i, ++attr2)
		{
		  DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr2->attr) << ' ');
		  switch(attr2->attr)
		  {
		    case DW_AT_inline:
		    {
		      LIBCWD_ASSERT(attr2->form == DW_FORM_data1);
		      constant_t inline_attr = read_constant(abstract_debug_info_ptr, attr2->form);
		      DoutDwarf(dc::bfd, "DW_AT_inline flag is " << inline_attr);
		      LIBCWD_ASSERT((inline_attr & 16) == 16);
		      if ((inline_attr & 32) == 0)
		      {
			((uint8_t*)abstract_debug_info_ptr)[-1] |= 32;
			DoutDwarf(dc::bfd, "DW_AT_inline flag set to " << (unsigned int)(((uint8_t*)abstract_debug_info_ptr)[-1]));
			++abstract_instances_referenced;
		        DoutDwarf(dc::bfd, "DW_AT_inline reference count set to " << abstract_instances_referenced);
		      }
		      saw_inline2 = true;
		      break;
		    }
		    case DW_AT_specification:
		    {  
		      LIBCWD_ASSERT( attr2->form != DW_FORM_ref_addr);	// Must be same compilation unit.
		      reference_t declaration_debug_info_ptr =
			  read_reference(abstract_debug_info_ptr, attr->form, debug_info_root, debug_info_start);
		      if (DEBUGDWARF)
			Debug(libcw_do.inc_indent(4));
		      dwarf_read(declaration_debug_info_ptr, code);
		      LIBCWD_ASSERT( code < abbrev_entries.size() );
		      abbrev_st& abbrev3(abbrev_entries[code]);
		      LIBCWD_ASSERT(abbrev3.tag == DW_TAG_subprogram);
		      attr_st* attr3 = abbrev3.attributes;
		      for (int i = 0; i < abbrev3.attributes_size; ++i, ++attr3)
		      {
			DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr3->attr) << ' ');
			switch(attr3->attr)
			{
			  case DW_AT_MIPS_linkage_name:
			    saw_linkage_name = true;
			    linkage_name = read_string(declaration_debug_info_ptr, attr3->form, debug_str);
			    break;
			  case DW_AT_name:
			    name = read_string(declaration_debug_info_ptr, attr3->form, debug_str);
			    break;
			  case DW_AT_inline:
			  case DW_AT_abstract_origin:
			    DoutFatal(dc::core, "Huh? DW_AT_specification contains " << print_DW_AT_name(attr3->attr));
			  default:
			    eat_form(declaration_debug_info_ptr, attr3->form DEBUGDWARF_OPT_COMMA(debug_str)
			      DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
			    break;
			}
		      }
		      if (DEBUGDWARF)
			Debug(libcw_do.dec_indent(4));
		      break;
		    }
		    default:
		      eat_form(abstract_debug_info_ptr, attr2->form DEBUGDWARF_OPT_COMMA(debug_str)
			DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
		      break;
                  }
		}
		LIBCWD_ASSERT( saw_inline2 );
		if (DEBUGDWARF)
		  Debug(libcw_do.dec_indent(4));
	        break;
	      }
	      case DW_AT_specification:
	      {  
	        LIBCWD_ASSERT( attr->form != DW_FORM_ref_addr);	// Must be same compilation unit.
		saw_specification = true;
	        reference_t declaration_debug_info_ptr =
		    read_reference(debug_info_ptr, attr->form, debug_info_root, debug_info_start);
		if (DEBUGDWARF)
		  Debug(libcw_do.inc_indent(4));
                dwarf_read(declaration_debug_info_ptr, code);
		LIBCWD_ASSERT( code < abbrev_entries.size() );
		abbrev_st& abbrev2(abbrev_entries[code]);
                LIBCWD_ASSERT(abbrev2.tag == DW_TAG_subprogram);
		attr_st* attr2 = abbrev2.attributes;
		for (int i = 0; i < abbrev2.attributes_size; ++i, ++attr2)
		{
		  DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr2->attr) << ' ');
		  switch(attr2->attr)
		  {
		    case DW_AT_MIPS_linkage_name:
		      saw_linkage_name = true;
		      linkage_name = read_string(declaration_debug_info_ptr, attr2->form, debug_str);
		      break;
		    case DW_AT_name:
		      name = read_string(declaration_debug_info_ptr, attr2->form, debug_str);
		      break;
		    case DW_AT_declaration:
		      saw_specification_declaration = true;
		      // Fall through.
		    default:
		      eat_form(declaration_debug_info_ptr, attr2->form DEBUGDWARF_OPT_COMMA(debug_str)
			DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
		      break;
		    case DW_AT_inline:
		    case DW_AT_abstract_origin:
		      DoutFatal(dc::core, "Huh? DW_AT_specification contains " << print_DW_AT_name(attr2->attr));
		  }
		}
		if (DEBUGDWARF)
		  Debug(libcw_do.dec_indent(4));
	        break;
	      }
	      case DW_AT_name:
		name = read_string(debug_info_ptr, attr->form, debug_str);
                break;
	      case DW_AT_declaration:
		saw_declaration = true;
		// Fall through.
	      default:
		eat_form(debug_info_ptr, attr->form DEBUGDWARF_OPT_COMMA(debug_str)
		  DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
	        break;
	    }
	  }
	  LIBCWD_ASSERT( saw_low_pc == saw_high_pc );
	  // Dies with DW_AT_inline are abstract.
	  LIBCWD_ASSERT( saw_inline || saw_low_pc != saw_declaration );
	  LIBCWD_ASSERT( !saw_low_pc || !saw_declaration );
	  LIBCWD_ASSERT( saw_specification == saw_specification_declaration );
	  if(!(saw_declaration || saw_inline || saw_linkage_name))
	    DoutDwarf(dc::bfd, "Missing DW_AT_MIPS_linkage_name for " << name);
	  if (saw_low_pc && saw_linkage_name)
	  {
	    DoutDwarf(dc::bfd, std::hex << "low_pc = " << low_pc <<
		std::hex << "; high_pc = " << high_pc << "; linkage_name = " << linkage_name);

	    // Look up the symbol.
	    bool found = false;
	    using namespace cwbfd;
	    function_symbols_ct const& function_symbols(object_file->get_function_symbols());
	    for(function_symbols_ct::const_iterator i2(function_symbols.begin()); i2 != function_symbols.end(); ++i2)
	    {
	      static unsigned int const setflags = BSF_FUNCTION;
	      symbol_ct const& symbol(*i2);
	      if ((symbol.get_symbol()->flags & setflags) == setflags)
	      {
		if (!strcmp(linkage_name, symbol.get_symbol()->name))
		{
		  DoutDwarf(dc::bfd, "Found symbol at " <<
		      std::hex << (address_t)symbol_start_addr(symbol.get_symbol()) <<
		      "; symbol value = " << std::hex << symbol.get_symbol()->value <<
		      "; symbol value + section offset = " <<
		      std::hex << symbol.get_symbol()->value + symbol.get_symbol()->section->offset <<
		      "; low_pc plus symbol size = " <<
		      std::hex << low_pc + symbol_size(symbol.get_symbol()));
		  found = true;
		  LIBCWD_ASSERT( low_pc == symbol.get_symbol()->value + symbol.get_symbol()->section->offset );
		  if (low_pc + symbol_size(symbol.get_symbol()) != high_pc )
		  {
		    // Alignment can cause the next function to start beyond the end of this function.
		    LIBCWD_ASSERT( low_pc + symbol_size(symbol.get_symbol()) > high_pc );
		    // The alignment is maximal 16.
		    LIBCWD_ASSERT( low_pc + symbol_size(symbol.get_symbol()) - high_pc < 16 );
		    // The last instruction is a ret or a jmp.
		    LIBCWD_ASSERT(*((unsigned char*)symbol_start_addr(symbol.get_symbol()) - 1 + high_pc - low_pc) == 0xc3 ||
			*((unsigned char*)symbol_start_addr(symbol.get_symbol()) - 3 + high_pc - low_pc) == 0xc2 ||
			*((unsigned char*)symbol_start_addr(symbol.get_symbol()) - 5 + high_pc - low_pc) == 0xe8 ||
			*((unsigned char*)symbol_start_addr(symbol.get_symbol()) - 5 + high_pc - low_pc) == 0xe9 ||
			*((unsigned char*)symbol_start_addr(symbol.get_symbol()) - 2 + high_pc - low_pc) == 0xeb);
		    // All remaining bytes should be alignment
		    for (unsigned char* p = (unsigned char*)symbol_start_addr(symbol.get_symbol()) + high_pc - low_pc;
			 p < (unsigned char*)symbol_start_addr(symbol.get_symbol()) + symbol_size(symbol.get_symbol());
			 ++p)
		    {
		      // nop
		      if (*p == 0x90)
			continue;
		      // lea    0x0(%esi,1),%esi
		      if (*p == 0x8d && p[1] == 0xb4 && p[2] == 0x26 && p[3] == 0x0 && p[4] == 0x0 && p[5] == 0x0 && p[6] == 0x0)
		      { p += 6; continue; }
		      DoutFatal(dc::core, "Unrecognizable alignment");
		    }
		  }
		}
	      }
	    }
	    if (!found)
	    {
	      DoutDwarf(dc::bfd, "DSO: " << object_file->get_bfd()->filename);
	      DoutFatal(dc::core, "Could not find DWARF function symbol in ELF symbol table.");
	    }
	  }
	}
	else if (abbrev.tag == DW_TAG_inlined_subroutine)
	{
	  string_t linkage_name = 0;
	  string_t name = 0;
	  bool saw_linkage_name = false;
	  constant_t inline_attr = 0;
	  for (int i = 0; i < abbrev.attributes_size; ++i, ++attr)
	  {
	    DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr->attr) << ' ');
	    switch(attr->attr)
	    {
	      case DW_AT_inline:
	        LIBCWD_ASSERT(attr->form == DW_FORM_data1);
	        inline_attr = read_constant(debug_info_ptr, attr->form);
		DoutDwarf(dc::bfd, "DW_AT_inline flag is " << inline_attr);
		((uint8_t*)debug_info_ptr)[-1] |= 16;
		DoutDwarf(dc::bfd, "DW_AT_inline flag set to " << (unsigned int)(((uint8_t*)debug_info_ptr)[-1]));
		++abstract_instances;
		DoutDwarf(dc::bfd, "DW_AT_inline count set to " << abstract_instances);
	        //saw_inline = true;
	        break;
	      case DW_AT_abstract_origin:
	      {
	        LIBCWD_ASSERT( attr->form != DW_FORM_ref_addr); // Must be same compilation unit.
		reference_t abstract_debug_info_ptr =
		    read_reference(debug_info_ptr, attr->form, debug_info_root, debug_info_start);
		if (DEBUGDWARF)
		  Debug(libcw_do.inc_indent(4));
		dwarf_read(abstract_debug_info_ptr, code);
		LIBCWD_ASSERT( code < abbrev_entries.size() );
		abbrev_st& abbrev2(abbrev_entries[code]);
		LIBCWD_ASSERT(abbrev2.tag == DW_TAG_subprogram);
		bool saw_inline2 = false;
		attr_st* attr2 = abbrev2.attributes;
		for (int i = 0; i < abbrev2.attributes_size; ++i, ++attr2)
		{
		  DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr2->attr) << ' ');
		  switch(attr2->attr)
		  {
		    case DW_AT_inline:
		    {
		      LIBCWD_ASSERT(attr2->form == DW_FORM_data1);
		      constant_t inline_attr = read_constant(abstract_debug_info_ptr, attr2->form);
		      DoutDwarf(dc::bfd, "DW_AT_inline flag is " << inline_attr);
		      if ((inline_attr & 32) == 0)
		      {
			((uint8_t*)abstract_debug_info_ptr)[-1] |= 32;
			DoutDwarf(dc::bfd, "DW_AT_inline flag set to " << (unsigned int)(((uint8_t*)abstract_debug_info_ptr)[-1]));
			++abstract_instances_referenced;
		        DoutDwarf(dc::bfd, "DW_AT_inline reference count set to " << abstract_instances_referenced);
		      }
		      saw_inline2 = true;
		      break;
		    }
		    case DW_AT_specification:
		    {  
		      LIBCWD_ASSERT( attr2->form != DW_FORM_ref_addr);	// Must be same compilation unit.
		      reference_t declaration_debug_info_ptr =
			  read_reference(abstract_debug_info_ptr, attr->form, debug_info_root, debug_info_start);
		      if (DEBUGDWARF)
			Debug(libcw_do.inc_indent(4));
		      dwarf_read(declaration_debug_info_ptr, code);
		      LIBCWD_ASSERT( code < abbrev_entries.size() );
		      abbrev_st& abbrev3(abbrev_entries[code]);
		      LIBCWD_ASSERT(abbrev3.tag == DW_TAG_subprogram);
		      attr_st* attr3 = abbrev3.attributes;
		      for (int i = 0; i < abbrev3.attributes_size; ++i, ++attr3)
		      {
			DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr3->attr) << ' ');
			switch(attr3->attr)
			{
			  case DW_AT_MIPS_linkage_name:
			    saw_linkage_name = true;
			    linkage_name = read_string(declaration_debug_info_ptr, attr3->form, debug_str);
			    break;
			  case DW_AT_name:
			    name = read_string(declaration_debug_info_ptr, attr3->form, debug_str);
			    break;
			  case DW_AT_inline:
			  case DW_AT_abstract_origin:
			    DoutFatal(dc::core, "Huh? DW_AT_specification contains " << print_DW_AT_name(attr3->attr));
			  default:
			    eat_form(declaration_debug_info_ptr, attr3->form DEBUGDWARF_OPT_COMMA(debug_str)
			      DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
			    break;
			}
		      }
		      if (DEBUGDWARF)
			Debug(libcw_do.dec_indent(4));
		      break;
		    }
		    default:
		      eat_form(abstract_debug_info_ptr, attr2->form DEBUGDWARF_OPT_COMMA(debug_str)
			DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
		      break;
                  }
		}
		LIBCWD_ASSERT( saw_inline2 );
		if (DEBUGDWARF)
		  Debug(libcw_do.dec_indent(4));
	        break;
	      }
	      default:
		eat_form(debug_info_ptr, attr->form DEBUGDWARF_OPT_COMMA(debug_str)
		    DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
	    }
	  }
	}
#endif
	else
	{
	  // The not-so-important tags - scan through them as fast as possible.
	  if (abbrev.fixed_size)
	  {
	    if (abbrev.starts_with_string)
	      eat_string(debug_info_ptr);
	    debug_info_ptr += abbrev.fixed_size;
	  }
	  else
	  {
	    // DW_AT_sibling is always the first attribute.
	    if (abbrev.attributes_size > 0 && attr->attr == DW_AT_sibling &&
	        abbrev.tag != DW_TAG_structure_type &&
	        abbrev.tag != DW_TAG_class_type &&
		abbrev.tag != DW_TAG_namespace &&
		abbrev.tag != DW_TAG_lexical_block 	// Do DW_TAG_lexical_block ever have a DW_AT_sibling?
	       )
	    {
	      LIBCWD_ASSERT(abbrev.has_children);
	      // Skip the children.
	      DoutDwarf(dc::bfd|continued_cf, "decoding DW_AT_sibling ");
	      debug_info_ptr = read_reference(debug_info_ptr, attr->form, debug_info_root, debug_info_start);
	      if (DEBUGDWARF)
		Debug(libcw_do.dec_indent(4));
	      continue;
	    }
	    for (int i = 0; i < abbrev.attributes_size; ++i, ++attr)
	    {
	      DoutDwarf(dc::bfd|continued_cf, "decoding " << print_DW_AT_name(attr->attr) << ' ');
	      eat_form(debug_info_ptr, attr->form DEBUGDWARF_OPT_COMMA(debug_str)
	          DEBUGDWARF_OPT_COMMA(debug_info_root - debug_info_start));
	    }
	  }
	}

        if (DEBUGDWARF)
	  Debug(libcw_do.dec_indent(4));
        if (abbrev.has_children)
	{
	  ++level;
          if (DEBUGDWARF)
	    Debug( libcw_do.marker().append("|", 1) );
	}

      }
      LIBCWD_ASSERT( debug_info_ptr == debug_info_ptr_end );
      LIBCWD_ASSERT( abstract_instances >= abstract_instances_referenced );
      if (abstract_instances > abstract_instances_referenced)
      {
	DoutDwarf(dc::bfd|flush_cf, "Redundant abstract instances in " << current_compilation_unit->get_source_file() <<
	    " : abstract_instances == " << abstract_instances <<
	    "; abstract_instances_referenced == " << abstract_instances_referenced);
      }
    }
    else
    {
      LIBCWD_TSD_DECLARATION;
      _private_::set_alloc_checking_on(LIBCWD_TSD);
      Dout(dc::warning, "DWARF version " << version << " is not understood by libcwd.");
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      debug_info_ptr += length - sizeof(version);
    }
  }

  delete [] debug_line;
  delete [] debug_info;
  delete [] debug_abbrev;
  delete [] debug_str;
  M_dwarf_debug_line_section_index = 0;
  if (!M_stabs_section_index)
    delete_hash_list();
  M_debug_info_loaded = true;
#if CWDEBUG_ALLOC
  saved_internal = __libcwd_tsd.internal;
  __libcwd_tsd.internal = false;
#endif
  Dout(dc::finish, "done");
#if CWDEBUG_ALLOC
  __libcwd_tsd.internal = saved_internal;
#endif
}

void objfile_ct::load_stabs()
{
#if DEBUGSTABS
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( !M_stabs_debug_info_loaded );
  M_stabs_debug_info_loaded = true;
  LIBCWD_ASSERT( M_sections[M_stabs_section_index].section_header().sh_entsize == sizeof(stab_st) );
#endif
  stab_st* stabs = (stab_st*)allocate_and_read_section(M_stabs_section_index);
#ifndef __sun__
  // The native Solaris 2.8 linker (ld) doesn't fill in the value of sh_link and also n_desc seems to have a different meaning.
  if (DEBUGSTABS)
  {
    LIBCWD_ASSERT( M_stabstr_section_index == M_sections[M_stabs_section_index].section_header().sh_link );
    LIBCWD_ASSERT( !strcmp(&M_section_header_string_table[M_sections[M_stabstr_section_index].section_header().sh_name], ".stabstr") );
    LIBCWD_ASSERT( stabs->n_desc == (Elfxx_Half)(M_sections[M_stabs_section_index].section_header().sh_size / M_sections[M_stabs_section_index].section_header().sh_entsize - 1) );
  }
#endif
  char* stabs_string_table = allocate_and_read_section(M_stabstr_section_index);
  if (DEBUGSTABS)
    Debug( libcw_do.inc_indent(4) );
  Elfxx_Addr func_addr = 0;
  object_files_string cur_dir;
  object_files_string cur_source;
  object_files_string cur_func;
  location_ct location(this);
  range_st range;
  bool skip_function = false;
  bool source_file_changed_and_we_didnt_copy_it_yet = true;
  bool source_file_changed_but_line_number_not_yet = true;
  object_files_string_set_ct::iterator last_source_iter;
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
	  if (filename[strlen(filename) - 1] == '/')
	  {
	    cur_dir.assign(filename);
	    DoutStabs(dc::bfd, ((stabs[j].n_type  == N_SO) ? "N_SO : \"" : "N_SOL: \"") << cur_dir << "\".");
	    break;
	  }
	  else
	    cur_source.assign(filename);
	}
	else
	{
	  if (filename[0] == '.' && filename[1] == '/')
	    filename += 2;
	  cur_source = catenate_path(cur_dir, filename);
	}
	cur_source += '\0';
	last_source_iter = M_source_files.insert(cur_source).first; 
	source_file_changed_and_we_didnt_copy_it_yet = source_file_changed_but_line_number_not_yet = true;
	DoutStabs(dc::bfd, ((stabs[j].n_type  == N_SO) ? "N_SO : \"" : "N_SOL: \"") << cur_source.data() << "\".");
	break;
      }
      case N_LBRAC:
      {
        DoutStabs(dc::bfd, "N_LBRAC: " << std::hex << stabs[j].n_value << '.');
	if (stabs[j].n_value == 0)		// Fix me: shouldn't be done (yet) while function start == source file start.
	  M_brac_relative_to_fun = true;
	break;
      }
      case N_RBRAC:	// We assume that the first N_RBRACE comes AFTER the last N_SLINE,
      			// see http://www.informatik.uni-frankfurt.de/doc/texi/stabs_2.html#SEC14
      {
	DoutStabs(dc::bfd, "N_RBRAC: " << std::hex << stabs[j].n_value << '.');
	if (location.is_valid_stabs())
	{
	  DoutStabs(dc::bfd, "N_RBRAC: " << "end at " << std::hex << stabs[j].n_value << '.');
	  range.size = 0;			// The closing brace has size 0...
	  if (!skip_function)
	    location.stabs_range(range);
	  skip_function = false;
	  location.invalidate();
	}
	break;
      }
      case N_FUN:
      {
	char const* fn;
	char const* fn_end;
	if (stabs[j].n_strx == 0
#ifdef __sun__
	    // A function should always be represented by an `F' symbol descriptor for a global (extern) function,
	    // and `f' for a static (local) function.  See http://www.informatik.uni-frankfurt.de/doc/texi/stabs_2.html#SEC12
	    // But on solaris there turn out to be non-standard N_FUN stabs that mean I don't know what.  Skip them here.
	    || *(fn = &stabs_string_table[stabs[j].n_strx]) == 0
	    || !(fn_end = strchr(fn, ':'))
	    || (fn_end[1] != 'F' && fn_end[1] != 'f')
#endif
	   )
	{
	  if (location.is_valid_stabs())	// Location is invalidated when we already processed the end of the function by N_RBRAC.
	  {
	    DoutStabs(dc::bfd, "N_FUN: " << "end at " << std::hex << stabs[j].n_value << '.');
	    range.size = func_addr + stabs[j].n_value - range.start;
	    if (!skip_function)
	      location.stabs_range(range);
	    skip_function = false;
	    location.invalidate();
	  }
	}
	else
	{
#ifndef __sun__
	  fn = &stabs_string_table[stabs[j].n_strx];
	  fn_end = strchr(fn, ':');
#if DEBUGSTABS
	  LIBCWD_ASSERT( fn_end && (fn_end[1] == 'F' || fn_end[1] == 'f') );
#endif
#endif
	  size_t fn_len = fn_end - fn;
	  cur_func.assign(fn, fn_len);
	  cur_func += '\0';
	  range.start = func_addr = stabs[j].n_value;
	  DoutStabs(dc::bfd, "N_FUN: " << std::hex << func_addr << " : \"" << &stabs_string_table[stabs[j].n_strx] << "\".");
	  if (func_addr == 0 && location.is_valid_stabs())
	  {
	    // Start of function is not given (bug in assembler?), try to find it by name:
	    uint32_t hash = elf_hash(reinterpret_cast<unsigned char const*>(fn), (unsigned char)':');
	    for(hash_list_st* p = M_hash_list[hash]; p; p = p->next)
	      if (!strncmp(p->name, fn, fn_len))
	      {
		range.start = func_addr = p->addr;
		if (!p->already_added)
		{
		  p->already_added = true;
		  break;
	        }
	      }
	    if (func_addr == 0)
	    {
	      // This can happen for .gnu.link_once section symbols: it might be that
	      // a function is present in the current object file but is not used;
	      // the dynamic linker has put it in the 'undefined' section and no
	      // address is known even though there is still this N_FUN entry.
	      skip_function = true;
	      location.invalidate();
	      break;
	    }
	    else
	      DoutStabs(dc::bfd, "Hash lookup: " << std::hex << range.start << '.');
	  }
#if DEBUGSTABS
	  else
	  {
	    Elfxx_Addr func_addr_test = 0;
	    uint32_t hash = elf_hash(reinterpret_cast<unsigned char const*>(fn), (unsigned char)':');
	    for(hash_list_st* p = M_hash_list[hash]; p; p = p->next)
	      if (!strncmp(p->name, fn, fn_len))
	      {
		func_addr_test = p->addr;
		if (!p->already_added || func_addr_test == func_addr)
		{
		  p->already_added = true;
		  break;
	        }
	      }
	    LIBCWD_ASSERT( func_addr_test == func_addr );
	  }
#endif
	  location.set_func_iter(M_function_names.insert(cur_func).first);
	  location.invalidate();	// See N_SLINE
	}
	break;
      }
      case N_SLINE:
	DoutStabs(dc::bfd, "N_SLINE: " << stabs[j].n_desc << " at " << std::hex << stabs[j].n_value << '.');
	if (stabs[j].n_value != 0)
	{
	  // Always false when function was changed since last line because location.invalidate() was called in that case.
	  // Catenate ranges with same location.
	  if (!source_file_changed_and_we_didnt_copy_it_yet && location.is_valid_stabs() && stabs[j].n_desc == location.get_line())
	    break;
	  range.size = func_addr + stabs[j].n_value - range.start;
	  // Delay one source/line change when there was no code since last source file change.
	  // The is apparently needed to deal with inlined functions.
	  if (range.size == 0 && source_file_changed_but_line_number_not_yet)
	  {
	    source_file_changed_but_line_number_not_yet = false;
	    break;
	  }
	  if (!skip_function && !source_file_changed_and_we_didnt_copy_it_yet)
	    location.stabs_range(range);
	  range.start += range.size;
	}
	// Store the source/line for the next range.
	location.set_source_iter(last_source_iter);
	location.set_line(stabs[j].n_desc);
	source_file_changed_and_we_didnt_copy_it_yet = false;
	source_file_changed_but_line_number_not_yet = false;
	break;
    }
  }
  if (DEBUGSTABS)
    Debug( libcw_do.dec_indent(4) );
  delete [] stabs;
  delete [] stabs_string_table;
  M_stabs_section_index = 0;
  if (!M_dwarf_debug_line_section_index)
    delete_hash_list();
  M_debug_info_loaded = true;
}

void objfile_ct::find_nearest_line(asymbol_st const* symbol, Elfxx_Addr offset, char const** file, char const** func, unsigned int* line LIBCWD_COMMA_TSD_PARAM)
{
  while (!M_debug_info_loaded)	// So we can use 'break'.
  {
    // The call to load_dwarf()/load_stabs() below can call malloc, causing us to recursively enter this function
    // for this object or another objfile_ct.
#if !LIBCWD_THREAD_SAFE
    if (M_inside_find_nearest_line) 		// Break loop caused by re-entry through a call to malloc.
    {
      *file = NULL;
      *func = symbol->name;
      *line = 0;
      return;
    }
    M_inside_find_nearest_line = true;
#else // LIBCWD_THREAD_SAFE
    // `S_thread_inside_find_nearest_line' is only *changed* inside the critical area
    // of `object_files_instance'.  Therefore, when it is set to our thread id at
    // this moment - then other threads can't change it.
    if (pthread_equal(S_thread_inside_find_nearest_line, pthread_self()))
    {
      // This thread set the lock.  Don't try to acquire it again...
      *file = NULL;
      *func = symbol->name;
      *line = 0;
      return;
    }
    // Ok, now we are sure that THIS thread doesn't hold the following lock, try to acquire it.
    // `object_files_string' and the STL containers using `_private_::object_files_allocator' in the following functions need this lock.
#if CWDEBUG_ALLOC
    int saved_internal;
#endif
    LIBCWD_DEFER_CLEANUP_PUSH(&_private_::rwlock_tct<object_files_instance>::cleanup, NULL);
    _private_::rwlock_tct<object_files_instance>::wrlock();
    // Now we acquired the lock, check again if another thread not already read the debug info.
    if (!M_debug_info_loaded)
    {
      S_thread_inside_find_nearest_line = pthread_self();
#endif
      libcwd::debug_ct::OnOffState state;
      libcwd::channel_ct::OnOffState state2;
      if (DEBUGSTABS || DEBUGDWARF ||
          (_private_::always_print_loading && !_private_::suppress_startup_msgs))
      {
	// We want debug output to BFD
	Debug( libcw_do.force_on(state) );
	Debug( dc::bfd.force_on(state2, "BFD") );
      }
      if (M_dwarf_debug_line_section_index)
	load_dwarf();
      else if (!M_stabs_section_index && !this->object_file->get_object_file()->has_no_debug_line_sections())
      {
	this->object_file->get_object_file()->set_has_no_debug_line_sections();
#if CWDEBUG_ALLOC
	int saved_internal2 = __libcwd_tsd.internal;
	__libcwd_tsd.internal = false;
#endif
	Dout( dc::warning, "Object file " << this->filename_str << " does not have debug info.  Address lookups inside "
	    "this object file will result in a function name only, not a source file location.");
#if CWDEBUG_ALLOC
	__libcwd_tsd.internal = saved_internal2;
#endif
      }
      if (M_stabs_section_index)
	load_stabs();
      if (DEBUGSTABS || DEBUGDWARF ||
          (_private_::always_print_loading && !_private_::suppress_startup_msgs))
      {
	Debug(dc::bfd.restore(state2));
	Debug(libcw_do.restore(state));
      }
      int saved_internal3 = _private_::set_library_call_on(LIBCWD_TSD);
      M_input_stream->close();
      _private_::set_library_call_off(saved_internal3 LIBCWD_COMMA_TSD);
#if LIBCWD_THREAD_SAFE
      S_thread_inside_find_nearest_line = (pthread_t) 0;
    }
    _private_::rwlock_tct<object_files_instance>::wrunlock();
#if CWDEBUG_ALLOC
    saved_internal = __libcwd_tsd.internal;
    __libcwd_tsd.internal = false;
#endif
    LIBCWD_CLEANUP_POP_RESTORE(false);
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = saved_internal;
#endif
#else
    M_inside_find_nearest_line = false;
#endif
    break;
  }
  range_st range;
  range.start = offset;
  range.size = 1;
  object_files_range_location_map_ct::const_iterator i(M_ranges.find(static_cast<range_st const>(range)));
  if (i == M_ranges.end() || ((*i).second.M_stabs_symbol && strcmp((*(*i).second.M_stabs_symbol_funcname_iter).data(), symbol->name)))
  {
    *file = NULL;
    *func = symbol->name;
    *line = 0;
  }
  else
  {
    *file = (*(*i).second.M_source_iter).data();
    if ((*i).second.M_stabs_symbol)
      *func = (*(*i).second.M_stabs_symbol_funcname_iter).data();
    else	// dwarf
      *func = symbol->name;
    *line = (*i).second.M_line;
  }
  return;
}

char* objfile_ct::allocate_and_read_section(int i)
{
  char* p = new char[M_sections[i].section_header().sh_size];
  LIBCWD_TSD_DECLARATION;
  int saved_internal = _private_::set_library_call_on(LIBCWD_TSD); 
  LIBCWD_DISABLE_CANCEL;
  M_input_stream->rdbuf()->pubseekpos(M_sections[i].section_header().sh_offset);
  M_input_stream->read(p, M_sections[i].section_header().sh_size);
  LIBCWD_ENABLE_CANCEL;
  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD); 
  return p;
}

void objfile_ct::register_range(location_st const& location, range_st const& range)
{
#if DEBUGSTABS || DEBUGDWARF
  LIBCWD_TSD_DECLARATION;
  if ((DEBUGDWARF && M_dwarf_debug_line_section_index)
      || (DEBUGSTABS && M_stabs_section_index))
  {
    if (doutdwarfon || doutstabson)
    {
      _private_::set_alloc_checking_on(LIBCWD_TSD);
      Dout(dc::bfd, std::hex << range.start << " - " << (range.start + range.size) << "; " << location << '.');
      _private_::set_alloc_checking_off(LIBCWD_TSD);
    }
  }
#endif
  std::pair<object_files_range_location_map_ct::iterator, bool>
      p(M_ranges.insert(std::pair<range_st, location_st>(range, location)));
  if (p.second)
    return;
  // Do some error recovery.
  std::pair<range_st, location_st> old(*p.first);		// Currently stored range.
  std::pair<range_st, location_st> nw(range, location);		// New range.
#if DEBUGSTABS || DEBUGDWARF
  Elfxx_Addr low = old.first.start;
  Elfxx_Addr high = old.first.start + old.first.size - 1;
  char const* low_mn = pc_mangled_function_name((char const*)this->object_file->get_lbase() + low);
  char const* high_mn = pc_mangled_function_name((char const*)this->object_file->get_lbase() + high);
  if (doutdwarfon || doutstabson)
  {
    if (low_mn != high_mn)
    {
      Dout(dc::bfd, std::hex << low << " == " << low_mn);
      Dout(dc::bfd, std::hex << high << " == " << high_mn);
    }
    else
      Dout(dc::bfd, "Function: " << low_mn);
  }
#endif
  bool need_old_reinsert = false;
  bool need_restore = false;
  range_st save_old_range = { 0, 0 };	// Avoid compiler warning.
#if DEBUGSTABS || DEBUGDWARF
  _private_::set_alloc_checking_on(LIBCWD_TSD);
#endif
  if (!location.M_stabs_symbol ||
      !(*p.first).second.M_stabs_symbol ||
      (*p.first).second.M_stabs_symbol_funcname_iter != location.M_stabs_symbol_funcname_iter)
  {
#if DEBUGSTABS || DEBUGDWARF
    if (doutdwarfon || doutstabson)
      Dout(dc::bfd, "WARNING: Collision between different functions (" << *p.first << ")!?");
#else
    ;
#endif
  }
  else
  {
    bool different_start = (*p.first).first.start != range.start;
    bool different_lines = (*p.first).second.M_line != location.M_line;
    if (different_start && different_lines)
    {
      // The most important reason for this, is a bug in GNU as version 2.3.90:
      // The last address increment just prior to the end of a function
      // is too large.  So, it is the end of the range that we cannot trust.
      //
      // Cut off the first range so at least we cover the total.
      // Before:
      //          |<--range1-->|
      //                   |<--range2-->|
      // After:
      //          |<range1>|<--range2-->|
      //
      // Now GNU as version 2.4.90 does not have this bug, but has another bug.
      // that bug is a lot harder to recover from, less frequently happening
      // and at the moment of writing not in use by many people; therefore no
      // attempt to recover from that has been added but instead this has been
      // reported as a bug to the binutils people.
      // See http://sources.redhat.com/ml/binutils/2003-10/msg00456.html

      if (nw.first.start < old.first.start)
      {
	//        + |<--old range-->|
	//      |<--new range------>?
	nw.first.size = old.first.start - nw.first.start;		// Cut short new range.
      }
      else
      {
	// |<--old range------>?
	//   ? |<--new range-->|
#if DEBUGSTABS || DEBUGDWARF
	if (doutdwarfon || doutstabson)
	  Dout(dc::bfd, "WARNING: New range overlaps old range, removing (" << *p.first << "),");
#else
	LIBCWD_TSD_DECLARATION;
#endif
	save_old_range = old.first;					// Backup.
	_private_::set_alloc_checking_off(LIBCWD_TSD);
	M_ranges.erase(p.first);
	_private_::set_alloc_checking_on(LIBCWD_TSD);
	need_restore = true;
	old.first.size = nw.first.start - old.first.start;		// Cut short old range.
	if (old.first.size > 0)
	{
#if DEBUGSTABS || DEBUGDWARF
	  if (doutdwarfon || doutstabson)
	    Dout(dc::bfd, "         ... and adding (" << old << ").");
#endif
	  need_old_reinsert = true;
	}
      }
#if DEBUGSTABS || DEBUGDWARF
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      if (doutdwarfon || doutstabson)
	Dout(dc::bfd, "WARNING: New range being added instead: " << nw);
      LIBCWD_ASSERT( nw.first.size > 0 );
      // Check that new range falls within one function.
      char const* nwlow_mn = pc_mangled_function_name((char const*)this->object_file->get_lbase() + nw.first.start);
      LIBCWD_ASSERT( nwlow_mn ==
          pc_mangled_function_name((char const*)this->object_file->get_lbase() + nw.first.start + nw.first.size - 1));
      // This is specific to the (only known) gas bug; check that this is at the end of function.
      // Note we only get here when the collision was with a range in the same source file, so we can
      // use the same this->object_file for the next function.
      LIBCWD_ASSERT( nwlow_mn !=
          pc_mangled_function_name((char const*)this->object_file->get_lbase() + nw.first.start + ((nw.first.start < old.first.start) ? (int)nw.first.size : -1)));
#endif
      if (!M_ranges.insert(nw).second)
      {
#if DEBUGSTABS || DEBUGDWARF
	if (doutdwarfon || doutstabson)
	  Dout(dc::bfd, "WARNING: Insertion of new range still failed");
#endif
	if (need_restore)
	{
	  old.first = save_old_range;
#if DEBUGSTABS || DEBUGDWARF
	  if (doutdwarfon || doutstabson)
	    Dout(dc::bfd, "         ... backing up to old situation (" << old << ").");
#endif
	  need_old_reinsert = true;
	}
      }
      if (need_old_reinsert)
      {
#if DEBUGSTABS || DEBUGDWARF
	p =
#endif
	  M_ranges.insert(old);
#if DEBUGSTABS || DEBUGDWARF
	if (!p.second)
	  if (doutdwarfon || doutstabson)
	    DoutFatal(dc::core, "Re-adding of shortened old range failed.");
#endif
      }
      return;
    }
#if DEBUGSTABS || DEBUGDWARF
    else if (doutdwarfon || doutstabson)
    {
      if ((*p.first).second.M_source_iter != location.M_source_iter)
	Dout(dc::bfd, "Collision with " << *p.first << ".");
      else if (different_start)
	Dout(dc::bfd, "WARNING: Different start for same function (" << *p.first << ")!?");
      else if (different_lines)
	Dout(dc::bfd, "WARNING: Different line numbers for overlapping range (" << *p.first << ")!?");
      else if ((*p.first).first.size != range.size)
	Dout(dc::bfd, "WARNING: Different sizes for same function!");
    }
#endif
  }
#if DEBUGSTABS || DEBUGDWARF
  _private_::set_alloc_checking_off(LIBCWD_TSD);
#endif
}

objfile_ct::objfile_ct() :
    M_section_header_string_table(NULL), M_sections(NULL), M_symbol_string_table(NULL),  M_dyn_symbol_string_table(NULL),
    M_symbols(NULL), M_number_of_symbols(0), M_symbol_table_type(0), M_hash_list(NULL)
{
}

void objfile_ct::initialize(char const* file_name)
{
  filename_str = file_name;
  LIBCWD_TSD_DECLARATION;
  int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
  Debug( libcw_do.off() );
  _private_::set_invisible_on(LIBCWD_TSD);
  M_input_stream = new std::ifstream;		// LEAK10
  M_input_stream->open(file_name);
  _private_::set_invisible_off(LIBCWD_TSD);
  Debug( libcw_do.on() );
  if (!M_input_stream->good())
    DoutFatal(dc::fatal|error_cf, "std::ifstream.open(\"" << file_name << "\")");
  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
  _private_::set_library_call_on(LIBCWD_TSD);
  *M_input_stream >> M_header;
  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
  LIBCWD_ASSERT(M_header.e_shentsize == sizeof(Elfxx_Shdr));
  if (M_header.e_shoff == 0 || M_header.e_shnum == 0)
    return;
  _private_::set_library_call_on(LIBCWD_TSD);
  M_input_stream->rdbuf()->pubseekpos(M_header.e_shoff);
  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
  Elfxx_Shdr* section_headers = new Elfxx_Shdr [M_header.e_shnum];
  _private_::set_library_call_on(LIBCWD_TSD);
  M_input_stream->read(reinterpret_cast<char*>(section_headers), M_header.e_shnum * sizeof(Elfxx_Shdr));
  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
  if (DEBUGELFXX)
  {
    _private_::set_alloc_checking_on(LIBCWD_TSD);
    Dout(dc::bfd, "Number of section headers: " << M_header.e_shnum);
    _private_::set_alloc_checking_off(LIBCWD_TSD);
  }
  LIBCWD_ASSERT( section_headers[M_header.e_shstrndx].sh_size > 0
      && section_headers[M_header.e_shstrndx].sh_size >= section_headers[M_header.e_shstrndx].sh_name );
  M_section_header_string_table = new char[section_headers[M_header.e_shstrndx].sh_size]; 	// LEAK11
  _private_::set_library_call_on(LIBCWD_TSD);
  M_input_stream->rdbuf()->pubseekpos(section_headers[M_header.e_shstrndx].sh_offset);
  M_input_stream->read(M_section_header_string_table, section_headers[M_header.e_shstrndx].sh_size);
  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
  LIBCWD_ASSERT( !strcmp(&M_section_header_string_table[section_headers[M_header.e_shstrndx].sh_name], ".shstrtab") );
  M_sections = new section_ct[M_header.e_shnum];						// LEAK11
  if (DEBUGELFXX)
    Debug( libcw_do.inc_indent(4) );
  M_debug_info_loaded = false;
  M_brac_relative_to_fun = false;
#if !LIBCWD_THREAD_SAFE
  M_inside_find_nearest_line = false;
#endif
#if DEBUGSTABS
  M_stabs_debug_info_loaded = false;
#endif
#if DEBUGDWARF
  M_dwarf_debug_info_loaded = false;
#endif
  M_stabs_section_index = 0;
  M_dwarf_debug_line_section_index = 0;
  M_dwarf_debug_str_section_index = 0;
  for(int i = 0; i < M_header.e_shnum; ++i)
  {
    if (DEBUGELFXX && section_headers[i].sh_name)
    {
      _private_::set_alloc_checking_on(LIBCWD_TSD);
      Dout(dc::bfd, "Section name: \"" << &M_section_header_string_table[section_headers[i].sh_name] << '"');
      _private_::set_alloc_checking_off(LIBCWD_TSD);
    }
    M_sections[i].init(M_section_header_string_table, section_headers[i]);
    if (!strcmp(M_sections[i].name, ".strtab"))
      M_symbol_string_table = allocate_and_read_section(i);
    else if (!strcmp(M_sections[i].name, ".dynstr"))
      M_dyn_symbol_string_table = allocate_and_read_section(i);
    else if (M_dwarf_debug_line_section_index == 0 && !strcmp(M_sections[i].name, ".stab"))
      M_stabs_section_index = i;
    else if (!strcmp(M_sections[i].name, ".stabstr"))
      M_stabstr_section_index = i;
    else if (!strcmp(M_sections[i].name, ".debug_line"))
      M_dwarf_debug_line_section_index = i;
    else if (!strcmp(M_sections[i].name, ".debug_abbrev"))
      M_dwarf_debug_abbrev_section_index = i;
    else if (!strcmp(M_sections[i].name, ".debug_info"))
      M_dwarf_debug_info_section_index = i;
    else if (!strcmp(M_sections[i].name, ".debug_str"))
      M_dwarf_debug_str_section_index = i;
    if (section_headers[i].sh_type == SHT_SYMTAB)
      M_is_stripped = false;
    if ((section_headers[i].sh_type == SHT_SYMTAB || section_headers[i].sh_type == SHT_DYNSYM)
        && section_headers[i].sh_size > 0)
    {
      M_has_syms = true;
      LIBCWD_ASSERT( section_headers[i].sh_entsize == sizeof(Elfxx_Sym) );
      LIBCWD_ASSERT( M_symbol_table_type != SHT_SYMTAB || section_headers[i].sh_type != SHT_SYMTAB);	// There should only be one SHT_SYMTAB.
      if (M_symbol_table_type != SHT_SYMTAB)								// If there is one, use it.
      {
	M_symbol_table_type = section_headers[i].sh_type;
	M_number_of_symbols = section_headers[i].sh_size / section_headers[i].sh_entsize;
      }
    }
  }
  if (DEBUGELFXX)
  {
    _private_::set_alloc_checking_on(LIBCWD_TSD);
    Debug( libcw_do.dec_indent(4) );
    Dout(dc::bfd, "Number of symbols: " << M_number_of_symbols);
    _private_::set_alloc_checking_off(LIBCWD_TSD);
  }
  delete [] section_headers;
}

bfd_st* bfd_st::openr(char const* file_name)
{
#if LIBCWD_THREAD_SAFE
  _private_::rwlock_tct<object_files_instance>::wrlock();
#endif
  objfile_ct* objfile = new objfile_ct;		// LEAK9
#if LIBCWD_THREAD_SAFE
  _private_::rwlock_tct<object_files_instance>::wrunlock();
#endif
  objfile->initialize(file_name);
  return objfile;
}

} // namespace elfxx

} // namespace libcwd

#if DEBUGSTABS || DEBUGDWARF
namespace libcwd {
  namespace cwbfd {
    bfile_ct* load_object_file(char const* name, void* l_addr);
  }
}

// This can be used to load and print an arbitrary object file (for debugging purposes).
void debug_load_object_file(char const* filename, bool /* shared */ = true)
{
  using namespace libcwd;
  cwbfd::bfile_ct* bfile = cwbfd::load_object_file(filename, 0);
  if (!bfile)
    return;
  LIBCWD_TSD_DECLARATION;
  libcwd::_private_::set_alloc_checking_off(LIBCWD_TSD);
  using namespace libcwd::elfxx;
  doutdwarfon = true;
  objfile_ct* of = static_cast<objfile_ct*>(bfile->get_bfd());
  if (of->M_dwarf_debug_line_section_index)
    of->load_dwarf();
  else if (!of->M_stabs_section_index && !of->object_file->get_object_file()->has_no_debug_line_sections())
  {
    of->object_file->get_object_file()->set_has_no_debug_line_sections();
    libcwd::_private_::set_alloc_checking_on(LIBCWD_TSD);
    Dout( dc::warning, "Object file " << of->filename_str << " does not have debug info.  Address lookups inside "
	"this object file will result in a function name only, not a source file location.");
    libcwd::_private_::set_alloc_checking_off(LIBCWD_TSD);
  }
  if (of->M_stabs_section_index)
    of->load_stabs();
  libcwd::_private_::set_alloc_checking_on(LIBCWD_TSD);
  of->M_input_stream->close();
  doutdwarfon = false;
}
#endif

#endif // CWDEBUG_LOCATION
