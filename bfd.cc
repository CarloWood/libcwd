// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "libcw/debugging_defs.h"

#ifdef DEBUGUSEBFD

#include "libcw/sys.h"
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <new>
#include <set>
#include <list>
#include <algo.h>
#include <link.h>
#include <strstream>
#include <string>
#include "libcw/h.h"
#include "libcw/debug.h"
#include "libcw/bfd.h"

RCSTAG_CC("$Id$")

extern "C" {
char* cplus_demangle (char const* mangled, int options);
}
#define DMGL_PARAMS 1
#define DMGL_ANSI 2

#ifdef DEBUG
#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif
    namespace channels {
      namespace dc {
	channel_ct const bfd("BFD");
      };
    };
#ifndef DEBUGNONAMESPACE
  };
};
#endif
#endif

#ifndef DEBUGNONAMESPACE
namespace {	// Local stuff
#endif

  void libcw_bfd_error_handler(char const* format, ...)
  {
    va_list vl;
    va_start(vl, format);
    Dout_vform(NAMESPACE_LIBCW_DEBUG::channels::dc::bfd, format, vl);
    va_end(vl);
  }

  ostream& operator<<(ostream& os, bfd_format _bfd_format)
  {
    os << bfd_format_string(_bfd_format);
    return os;
  }

  ostream& operator<<(ostream& os, bfd_flavour _bfd_flavour)
  {
    switch(_bfd_flavour)
    {
      case bfd_target_unknown_flavour:
	os << "unknown";
	break;
      case bfd_target_aout_flavour:
	os << "aout";
	break;
      case bfd_target_coff_flavour:
	os << "coff";
	break;
      case bfd_target_ecoff_flavour:
	os << "ecoff";
	break;
      case bfd_target_elf_flavour:
	os << "elf";
	break;
      default:
	os << "other";
	break;
    }
    return os;
  }

  ostream& operator<<(ostream& os, bfd_target const& _bfd_target)
  {
    os << _bfd_target.name << " (" << _bfd_target.flavour << ')';
    return os;
  }

  ostream& operator<<(ostream& os, asection const& _asection)
  {
    os << _asection.name;
    return os;
  }

  void print_section(bfd* abfd, asection* sect, PTR obj)
  {
    (*(ostream*)obj) << "\n\tSection: " << *sect;
  }

  ostream& operator<<(ostream& os, bfd const& _bfd)
  {
    os << "\n\tFilename: " << _bfd.filename;
    os << "\n\tTarget  : " << *_bfd.xvec;
    os << "\n\tFormat  : " << _bfd.format;
    os << "\n\tStart   : 0x" << hex << _bfd.start_address << dec;

    bfd_map_over_sections(const_cast<bfd*>(&_bfd), print_section, (PTR)&os);

    return os;
  }

  ostream& operator<<(ostream& os, alent const& _alent)
  {
    os << _alent.line_number;
    return os;
  }

  class symbol_key_ct {
  private:
    asymbol* symbol;
  public:
    symbol_key_ct(asymbol* p) : symbol(p) { }
    asymbol const* get_symbol(void) const { return symbol; }
    bool operator==(symbol_key_ct const& UNUSED(b)) const
	{ DoutFatal(dc::core, "Calling operator=="); }
    friend struct symbol_key_greater;
  };

  struct symbol_key_greater {
    // Returns true when the start of a lays behond the end of b (ie, no overlap).
    bool operator()(symbol_key_ct const& a, symbol_key_ct const& b) const;
  };

  typedef set<symbol_key_ct, symbol_key_greater> function_symbols_ct;

  void process_section(bfd* abfd, asection* sect, PTR obj);

  class object_file_ct {
  private:
    bfd* abfd;
    void* lbase;
    asymbol** symbol_table;
    long number_of_symbols;
    function_symbols_ct function_symbols;
  public:
    object_file_ct(void) : lbase(0) { }
    object_file_ct(char const* filename, void* base);
    friend void process_section(bfd* abfd, asection* sect, PTR obj);

    bfd* get_bfd(void) const { return abfd; }
    void* const get_lbase(void) const { return lbase; }
    asymbol** get_symbol_table(void) const { return symbol_table; }
    long get_number_of_symbols(void) const { return number_of_symbols; }
    function_symbols_ct& get_function_symbols(void) { return function_symbols; }
    function_symbols_ct const& get_function_symbols(void) const { return function_symbols; }
  };

  inline const PTR /* Yuk, PTR is a #define! Must put the `const' in front of it */
  symbol_start_addr(asymbol const* s)
  {
    return s->value + bfd_get_section(s)->vma
	+ reinterpret_cast<char const*>(reinterpret_cast<object_file_ct const*>(bfd_asymbol_bfd(s)->usrdata)->get_lbase());
  }

  inline size_t symbol_size(asymbol const* s)
  {
    return reinterpret_cast<size_t>(s->udata.p);
  }

  bool symbol_key_greater::operator()(symbol_key_ct const& a, symbol_key_ct const& b) const
  {
    return symbol_start_addr(a.symbol) >= reinterpret_cast<char const*>(symbol_start_addr(b.symbol)) + symbol_size(b.symbol);
  }

  void process_section(bfd* abfd, asection* sect, PTR obj)
  {
    object_file_ct* object_file = (object_file_ct*)obj;
    long storage_needed = bfd_get_reloc_upper_bound(abfd, sect);
    if (storage_needed < 0)
      DoutFatal(dc::bfd, "bfd_get_reloc_upper_bound: " << bfd_errmsg(bfd_get_error()));
    else if (storage_needed > 0)
    {
      arelent** relocation_table = (arelent**) malloc (storage_needed);
      long number_of_relocations = bfd_canonicalize_reloc(abfd, sect, relocation_table, object_file->symbol_table);
      if (number_of_relocations < 0)
	DoutFatal(dc::bfd, "bfd_canonicalize_reloc: " << bfd_errmsg(bfd_get_error()));
      else if (number_of_relocations > 0)
      {
	Dout(dc::warning, abfd->filename << ": " << sect->name << " section contains relocation data; libcw doesn't know how to deal with that");
	Dout(dc::bfd, sect->name << ": Number of relocations: " << number_of_relocations);
      }
    }

    for (int i = 0; i < object_file->number_of_symbols; ++i)
    {
      asymbol* p = object_file->symbol_table[i];

      if (sect == bfd_get_section(p))
	object_file->function_symbols.insert(function_symbols_ct::key_type(p));
    }
  }

  // Global object (but libcwd must stay independent of stuff in libcw/kernel, so we don't use Global<>)
  typedef list<object_file_ct*> object_files_ct;
  static char object_files_instance_[sizeof(object_files_ct)] __attribute__((__aligned__));
  object_files_ct& object_files(void) { return *reinterpret_cast<object_files_ct*>(object_files_instance_); }

  object_file_ct* find_object_file(void const* addr)
  {
    object_files_ct::iterator i(object_files().begin());
    for(; i != object_files().end(); ++i)
      if ((void*)(*i)->get_lbase() < addr)
	break;
    return (i != object_files().end()) ? (*i) : NULL;
  }

  struct symbol_less {
    bool operator()(asymbol const* a, asymbol const* b) const
    {
      if (a == b)
	return false;
      if (bfd_get_section(a)->vma + a->value < bfd_get_section(b)->vma + b->value)
	return true;
      else if (bfd_get_section(a)->vma + a->value > bfd_get_section(b)->vma + b->value)
	return false;
      else if (!(a->flags & BSF_FUNCTION) && (b->flags & BSF_FUNCTION))
	return true;
      else if ((a->flags & BSF_FUNCTION) && !(b->flags & BSF_FUNCTION))
	return false;
      else if (*a->name == '.')
	return true;
      else if (*b->name == '.')
	return false;
      else if (!strcmp(a->name, "gcc2_compiled."))
	return true;
      else if (!strcmp(b->name, "gcc2_compiled."))
	return false;
      else if (!(a->flags & BSF_GLOBAL) && (b->flags & BSF_GLOBAL))
	return true;
      else if ((a->flags & BSF_GLOBAL) && !(b->flags & BSF_GLOBAL))
	return false;
      else if (!(a->flags & BSF_LOCAL) && (b->flags & BSF_LOCAL))
	return true;
      else if ((a->flags & BSF_LOCAL) && !(b->flags & BSF_LOCAL))
	return false;
      else if (!(a->flags & BSF_OBJECT) && (b->flags & BSF_OBJECT))
	return true;
      else if ((a->flags & BSF_OBJECT) && !(b->flags & BSF_OBJECT))
	return false;
      /* Lets hope that IF it matters, that a long name is more important ;) */
      return (strlen(a->name) < strlen(b->name));
    }
  };

  object_file_ct::object_file_ct(char const* filename, void* base) : lbase(base)
  {
    abfd = bfd_openr(filename, NULL);
    if (!abfd)
      DoutFatal(dc::bfd, "bfd_openr: " << bfd_errmsg(bfd_get_error()));
    abfd->cacheable = bfd_tttrue;
    abfd->usrdata = (PTR)this;

    if (!bfd_check_format (abfd, bfd_object))
    {
      bfd_close(abfd);
      DoutFatal(dc::bfd, '"' << filename << "\": not in executable format: " << bfd_errmsg(bfd_get_error()));
    }

    //Dout(dc::bfd, "Opened BFD: " << *abfd);

    long storage_needed = bfd_get_symtab_upper_bound (abfd);
    if (storage_needed < 0)
      DoutFatal(dc::bfd, "bfd_get_symtab_upper_bound: " << bfd_errmsg(bfd_get_error()));

    symbol_table = (asymbol**) malloc(storage_needed);
    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
    if (number_of_symbols < 0)
      DoutFatal(dc::bfd, "bfd_canonicalize_symtab: " << bfd_errmsg(bfd_get_error()));
    //Dout(dc::bfd, "Number of symbols: " << number_of_symbols);

    if (number_of_symbols > 0)
    {
      // Throw away symbols that we don't want
      asymbol** se = &symbol_table[number_of_symbols - 1];
      for (asymbol** s = symbol_table; s <= se;)
      {
	if ((*s)->name == 0 || (*s)->value == 0 || ((*s)->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_FUNCTION|BSF_OBJECT)) == 0
	    || ((*s)->flags & (BSF_DEBUGGING|BSF_SECTION_SYM|BSF_OLD_COMMON|BSF_NOT_AT_END|
			       BSF_CONSTRUCTOR|BSF_WARNING|BSF_INDIRECT|BSF_FILE|BSF_DYNAMIC)) != 0
	    || ((*s)->flags == BSF_LOCAL && (*s)->name[0] == '.')	// Some labels have a size(?!), their flags seem to be always 1
	    || bfd_is_und_section(bfd_get_section(*s))
	    || bfd_is_abs_section(bfd_get_section(*s))
	    || bfd_is_com_section(bfd_get_section(*s))
	    || bfd_is_ind_section(bfd_get_section(*s)))
	{
	  *s = *se--;
	  --number_of_symbols;
	}
	else
	  ++s;
      }

      // Sort the symbol table in order of start address.
      sort(symbol_table, &symbol_table[number_of_symbols], symbol_less());

      for (int i = 0; i < number_of_symbols - 1; ++i)
      {
	symbol_table[i]->udata.p =
	    reinterpret_cast<void*>((size_t)((char*)symbol_start_addr(symbol_table[i + 1])
					    - (char*)symbol_start_addr(symbol_table[i])));
      }

      // Use reasonable size for last one:
      symbol_table[number_of_symbols - 1]->udata.p = reinterpret_cast<void*>((size_t)100000);
    }

    bfd_map_over_sections(abfd, process_section, (PTR)this);

    object_files().push_back(this);
  }

  struct object_file_greater {
    bool operator()(object_file_ct const* a, object_file_ct const* b) const { return a->get_lbase() > b->get_lbase(); }
  };

#ifndef DEBUGNONAMESPACE
};	// End namespace local stuff
#endif

//
// Writing location_st to an ostream.
//
ostream& operator<<(ostream& o, location_st const& loc)
{
  if (!loc.file)
    o << "<unknown location>";
  else
  {
    char const* filename = strrchr(loc.file, '/');
    o << (filename ? filename + 1 : loc.file) << ':' << loc.line;
  }
  return o;
}

static bool initialized = false;

static int libcw_bfd_init(void)
{
  // Initialize object files list
  new (object_files_instance_) object_files_ct;

  bfd_init();

  // Get the full path and name of executable
  char fullpath[512];
  int cmdline = open("/proc/self/cmdline", O_RDONLY);
  int len = read(cmdline, fullpath, sizeof(fullpath));
  if (len == -1)
    DoutFatal( error_cf, "libcw_bfd_init: read" );
  fullpath[len] = 0;
  bfd_set_error_program_name(fullpath);
  len = readlink("/proc/self/exe", fullpath, sizeof(fullpath) - 1);
  if (len == -1)
    DoutFatal( error_cf, "libcw_bfd_init: readlink" );
  fullpath[len] = 0;
  if (len == sizeof(fullpath) - 1)
    DoutFatal( dc::fatal, "libcw_bfd_init: executable name too long (\"" << fullpath << "\")" );

  bfd_set_error_handler(libcw_bfd_error_handler);

  // Load executable
  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug symbols from " << fullpath << "... ");
  new object_file_ct(fullpath, 0);
  Dout(dc::finish, "done");

  // Load all shared objects
  extern struct link_map* _dl_loaded;
  for (struct link_map* l = _dl_loaded; l; l = l->l_next)
    if (l->l_addr)
    {
      Dout(dc::bfd|continued_cf, "Loading debug symbols from " << l->l_name << "... ");
      new object_file_ct(l->l_name, reinterpret_cast<void*>(l->l_addr));
      Dout(dc::finish, "done");
    }

  object_files().sort(object_file_greater());

  initialized = true;

#if 0
  for (object_files_ct::iterator i(object_files().begin()); i != object_files().end(); ++i)
    if (!strcmp((*i)->get_bfd()->filename, "/home/carlo/c++/libcw/lib/libcw.so.0"))
    {
      asymbol** symbol_table = (*i)->get_symbol_table();
      for (long n = (*i)->get_number_of_symbols(); n > 0; --n)
      {
	cout << symbol_table[n]->name << ' ' <<
		symbol_table[n]->value << ' ' <<
	 oct << symbol_table[n]->flags << ' ' <<
	 hex << symbol_start_addr(symbol_table[n]) << ' ' <<
	 dec << symbol_size(symbol_table[n]) << endl;
      }
    }
#endif

  return 0;
}

static asymbol const* libcw_bfd_pc_symbol(void const* addr, object_file_ct const* object_file)
{
  static asymbol dummy_symbol;
  static asection dummy_section;
  if (object_file)
  {
    // Make symbol_start_addr(&dummy_symbol) and symbol_size(&dummy_symbol) return the correct value
    bfd_asymbol_bfd(&dummy_symbol) = object_file->get_bfd();
    dummy_symbol.section = &dummy_section;	// Has dummy_section.vma == 0.  Use dummy_symbol.value to store (value + vma):
    dummy_symbol.value = reinterpret_cast<char const*>(addr) - reinterpret_cast<char const*>(object_file->get_lbase());
    dummy_symbol.udata.i = 1;
    function_symbols_ct::iterator i(object_file->get_function_symbols().find(symbol_key_ct(&dummy_symbol)));
    if (i != object_file->get_function_symbols().end())
    {
      asymbol const* p = (*i).get_symbol();
      if (addr < (char*)symbol_start_addr(p) + symbol_size(p))
        return p;
    }
    Dout(dc::bfd, "No symbol found: " << hex << addr);
  }
#ifdef DEBUG
  else
    Dout(dc::bfd, "No source file found: " << hex << addr);
#endif
  return NULL;
}

//
// Find the mangled function name of the address `addr'.
// This function is successful in cases where libcw_bfd_pc_location fails.
//
char const* libcw_bfd_pc_function_name(void const* addr)
{
  if (!initialized)
  {
#ifdef DEBUGMALLOC
    set_alloc_checking_off();
#endif
    libcw_bfd_init();
#ifdef DEBUGMALLOC
    set_alloc_checking_on();
#endif
  }

  asymbol const* p = libcw_bfd_pc_symbol(addr, find_object_file(addr));

  if (!p)
    return NULL;

  return p->name;
}

//
// Find source file, (mangled) function name and line number of the address `addr'.
//
location_st libcw_bfd_pc_location(void const* addr) return location
{
  if (!initialized)
  {
#ifdef DEBUGMALLOC
    set_alloc_checking_off();
#endif
    libcw_bfd_init();
#ifdef DEBUGMALLOC
    set_alloc_checking_on();
#endif
  }

  object_file_ct const* object_file = find_object_file(addr);
  asymbol const* p = libcw_bfd_pc_symbol(addr, object_file);
  if (p)
  {
    asection* sect = bfd_get_section(p);
    bfd *abfd = bfd_asymbol_bfd(p);
    char const* file;
    bfd_find_nearest_line(abfd, sect, const_cast<asymbol**>(object_file->get_symbol_table()),
	(char*)addr - sect->vma - (char*)((object_file_ct*)abfd->usrdata)->get_lbase(), &file, &location.func, &location.line);

    if (file && location.line)	// When line is 0, it turns out that `file' is nonsense.
    {
      size_t len = strlen(file);
      char* filename = (char*)malloc(len + 1);
      AllocTag(filename, "location_st::file");
      location.file = strcpy(filename, file);
    }
    else
      location.file = NULL;

    if (p->name && p->name == location.func)	// bfd_find_nearest_line() failed if the latter is not true
    {
      location.file = NULL;
      location.line = 0;
      location.func = p->name;
    }

#ifdef DEBUG
    /* Sanity check */
    if (!p->name || location.line == 0)
    {
      if (p->name)
      {
	char* demangled_name = cplus_demangle(p->name, DMGL_PARAMS | DMGL_ANSI);
	Dout(dc::bfd, "Warning: Address " << hex << addr << " in section " << sect->name <<
	    " does not have a line number, perhaps the unit containing the function");
	Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << (demangled_name ? demangled_name : p->name) <<
	    "' wasn't compiled with CFLAGS=-g");
      }
      else
	Dout(dc::bfd, "Warning: Address in section " << sect->name << " does not contain a function");
    }
    else
      Dout(dc::bfd, hex << addr << dec << " is at (" << location << ')');
#endif
    return location;
  }
  location.line = 0;
  location.func = "<unknown function>";
  location.file = NULL;

  return location;
}

#endif // !DEBUGUSEBFD
