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

#include <libcw/debugging_defs.h>

#ifdef DEBUGUSEBFD

#include <libcw/sys.h>
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
#include <fstream>
#include <sys/param.h>          // Needed for realpath(3)
#include <unistd.h>             // Needed for getpid(2) and realpath(3)
#ifdef __FreeBSD__
#include <stdlib.h>             // Needed for realpath(3)
#endif
#include <libcw/h.h>
#include <libcw/debug.h>
#include <libcw/bfd.h>
#include <libcw/exec_prog.h>
#include <libcw/cwprint.h>

#undef DEBUGDEBUGBFD
#ifdef DEBUGDEBUGBFD
#include <iomanip>
#endif

RCSTAG_CC("$Id$")

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
    Dout_vform(dc::bfd, format, vl);
    va_end(vl);
  }

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

  typedef set<symbol_ct, symbol_key_greater> function_symbols_ct;

  class object_file_ct {
  private:
    bfd* abfd;
    void* const lbase;
    asymbol** symbol_table;
    long number_of_symbols;
    function_symbols_ct function_symbols;
  public:
    object_file_ct(void) : lbase(0) { }
    object_file_ct(char const* filename, void* base);

    bfd* get_bfd(void) const { return abfd; }
    void* const get_lbase(void) const { return lbase; }
    asymbol** get_symbol_table(void) const { return symbol_table; }
    long get_number_of_symbols(void) const { return number_of_symbols; }
    function_symbols_ct& get_function_symbols(void) { return function_symbols; }
    function_symbols_ct const& get_function_symbols(void) const { return function_symbols; }
  };

  typedef PTR addr_ptr_t;
  typedef const PTR addr_const_ptr_t;	// Warning: PTR is a macro, must put `const' in front of it

  inline addr_const_ptr_t
  symbol_start_addr(asymbol const* s)
  {
    return s->value + bfd_get_section(s)->vma
	+ reinterpret_cast<char const*>(reinterpret_cast<object_file_ct const*>(bfd_asymbol_bfd(s)->usrdata)->get_lbase());
  }

  inline size_t symbol_size(asymbol const* s)
  {
    return reinterpret_cast<size_t>(s->udata.p);
  }

  inline size_t& symbol_size(asymbol* s)
  {
    return *reinterpret_cast<size_t*>(&s->udata.p);
  }

  bool symbol_key_greater::operator()(symbol_ct const& a, symbol_ct const& b) const
  {
    return symbol_start_addr(a.symbol) >= reinterpret_cast<char const*>(symbol_start_addr(b.symbol)) + symbol_size(b.symbol);
  }

  // Global object (but libcwd must stay independent of stuff in libcw/kernel, so we don't use Global<>)
  typedef list<object_file_ct*> object_files_ct;
  static char object_files_instance_[sizeof(object_files_ct)] __attribute__((__aligned__));
  object_files_ct& object_files(void) { return *reinterpret_cast<object_files_ct*>(object_files_instance_); }

  object_file_ct* find_object_file(void const* addr)
  {
    object_files_ct::iterator i(object_files().begin());
    for(; i != object_files().end(); ++i)
      if ((*i)->get_lbase() < addr)
	break;
    return (i != object_files().end()) ? (*i) : NULL;
  }

  object_file_ct* find_object_file(bfd const* abfd)
  {
    object_files_ct::iterator i(object_files().begin());
    for(; i != object_files().end(); ++i)
      if ((*i)->get_bfd() == abfd)
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
      // Lets hope that IF it matters, that a long name is more important ;)
      return (strlen(a->name) < strlen(b->name));
    }
  };

  object_file_ct::object_file_ct(char const* filename, void* base) : lbase(base)
  {
    abfd = bfd_openr(filename, NULL);
    if (!abfd)
      DoutFatal(dc::bfd, "bfd_openr: " << bfd_errmsg(bfd_get_error()));
    abfd->cacheable = bfd_tttrue;
    abfd->usrdata = (addr_ptr_t)this;

    if (bfd_check_format(abfd, bfd_archive))
    {
      bfd_close(abfd);
      DoutFatal(dc::bfd, filename << ": can not get addresses from archive: " << bfd_errmsg(bfd_get_error()));
    }
    char** matching;
    if (!bfd_check_format_matches(abfd, bfd_object, &matching))
    {
      if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
      {
        Dout(dc::warning, "bfd_check_format_matches: ambiguously object format, recognized formats: " << cwprint(::libcw::debug::environment_ct(matching)));
	free(matching);
      }
      DoutFatal(dc::fatal, filename << ": can not get addresses from object file: " << bfd_errmsg(bfd_get_error()));
    }

    if (!(bfd_get_file_flags(abfd) & HAS_SYMS))
    {
      Dout(dc::warning, filename << " has no symbols, skipping.");
      bfd_close(abfd);
      delete this;
      return;
    }

    long storage_needed = bfd_get_symtab_upper_bound (abfd);
    if (storage_needed < 0)
      DoutFatal(dc::bfd, "bfd_get_symtab_upper_bound: " << bfd_errmsg(bfd_get_error()));

    symbol_table = (asymbol**) malloc(storage_needed);
    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
    if (number_of_symbols < 0)
      DoutFatal(dc::bfd, "bfd_canonicalize_symtab: " << bfd_errmsg(bfd_get_error()));

    if (number_of_symbols > 0)
    {
      // Sort the symbol table in order of start address.
      sort(symbol_table, &symbol_table[number_of_symbols], symbol_less());

      // Calculate sizes for every symbol (forced to be larger than 0)
      for (int i = 0; i < number_of_symbols - 1; ++i)
      {
	int j;
	for (j = i + 1; j < number_of_symbols - 1; ++j)
	  if (symbol_start_addr(symbol_table[j]) != symbol_start_addr(symbol_table[i]))
	    break;
	symbol_size(symbol_table[i]) = (char*)symbol_start_addr(symbol_table[j]) - (char*)symbol_start_addr(symbol_table[i]);
      }

      // Use reasonable size for last one:
      symbol_size(symbol_table[number_of_symbols - 1]) = 100000;

      // Throw away useless or meaningless symbols
      asymbol** se = &symbol_table[number_of_symbols - 1];
      for (asymbol** s = symbol_table; s <= se;)
      {
	if ((*s)->name == 0 || ((*s)->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_FUNCTION|BSF_OBJECT)) == 0
	    || ((*s)->flags & (BSF_DEBUGGING|BSF_SECTION_SYM|BSF_OLD_COMMON|BSF_NOT_AT_END|
			       BSF_CONSTRUCTOR|BSF_WARNING|BSF_INDIRECT|BSF_FILE|BSF_DYNAMIC)) != 0
	    || ((*s)->flags == BSF_LOCAL && (*s)->name[0] == '.')	// Some labels have a size(?!), their flags seem to be always 1
	    || bfd_is_abs_section(bfd_get_section(*s))
	    || bfd_is_com_section(bfd_get_section(*s))
	    || bfd_is_ind_section(bfd_get_section(*s)))
	{
	  *s = *se--;
	  --number_of_symbols;
	}
	else
	{
	  function_symbols.insert(function_symbols_ct::key_type(*s, !bfd_is_und_section(bfd_get_section(*s))));
	  ++s;
        }
      }

      if (symbol_size(symbol_table[number_of_symbols - 1]) == 100000)
        Dout( dc::warning, "Unknown size of symbol " << symbol_table[number_of_symbols - 1]->name);
    }

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

// 
// Find the full path to the current running process.
// This needs to work before we reach main, therefore
// it uses the /proc filesystem.  In order to be as
// portable as possible the most general trick is used:
// reading "/proc/PID/cmdline".  It expects that the
// name of the program (argv[0] in main()) is returned
// as a zero terminated string when reading this file.
// 

string full_path_to_executable(void) return result
{
  size_t const max_proc_path = sizeof("/proc/65535/cmdline\0");
  char proc_path[max_proc_path];
  ostrstream proc_path_str(proc_path, max_proc_path);

  proc_path_str << "/proc/" << getpid() << "/cmdline" << ends;
  ifstream proc_file(proc_path);

  if (!proc_file)
    DoutFatal(error_cf, "ifstream proc_file(\"" << proc_path << "\")");

  string argv0;
  proc_file >> argv0;
  proc_file.close();

  char full_path_buf[MAXPATHLEN];
  char* full_path = realpath(argv0.data(), full_path_buf);

  if (!full_path)
    DoutFatal(error_cf, "realpath(\"" << argv0.data() << "\", full_path_buf)");

  result = full_path;
}

static bool initialized = false;

#if 1 //ndef __linux /* See below */
namespace {

struct my_link_map {
  void* l_addr;
  char l_name[MAXPATHLEN];
  my_link_map(char const* start, size_t len, void* addr) : l_addr(addr)
  {
    if (len >= MAXPATHLEN)
      len = MAXPATHLEN - 1;
    strncpy(l_name, start, len);
    l_name[len] = 0;
  }
};

vector<my_link_map> shared_libs;

int decode(char const* buf, size_t len)
{
  for (char const* p = buf; p < &buf[len]; ++p)
    if (p[0] == '=' && p[1] == '>' && p[2] == ' ' && p[3] == '/')
    {
      p += 3;
      char const* q;
      for (q = p; q < &buf[len] && *q > ' '; ++q);
      for (char const* r = q; r < &buf[len]; ++r)
        if (r[0] == '(' && r[1] == '0' && r[2] == 'x')
        {
          char* s;
          void* addr = reinterpret_cast<void*>(strtol(++r, &s, 0));
          shared_libs.push_back(my_link_map(p, q - p, addr));
	  break;
        }
      break;
    }
  return 0;
}

}; // anonymous namespace
#endif

static int libcw_bfd_init(void)
{
#if defined(DEBUGDEBUG) || defined(DEBUGDEBUGBFD)
  static bool entered = false;
  if (entered)
    DoutFatal(dc::core, "Bug in libcwd: libcw_bfd_init() called twice or recursively entering itself!  Please submit a full bug report to libcw@alinoe.com.");
  entered = true;
#endif

  // Initialize object files list
  new (object_files_instance_) object_files_ct;

  bfd_init();

  // Get the full path and name of executable
  static string const fullpath(full_path_to_executable());		// Must be static because bfd keeps a pointer to its data()

  bfd_set_error_program_name(fullpath.data() + fullpath.find_last_of('/') + 1);
  bfd_set_error_handler(libcw_bfd_error_handler);

  // Load executable
  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug symbols from " << fullpath << "... ");
  object_file_ct* object_file = new object_file_ct(fullpath.data(), 0);
#ifdef DEBUG
  if (object_file->get_number_of_symbols() > 0)
    Dout(dc::finish, "done (" << dec << object_file->get_number_of_symbols() << " symbols)");
  else
    Dout(dc::finish, "No symbols found");
#endif

  // Load all shared objects
#if 0 //def __linux /* Commented out because it gives different results than ldd ?! */
  // This is faster because we don't start a child process
  extern struct link_map* _dl_loaded;
  for (struct link_map* l = _dl_loaded; l; l = l->l_next)
  {
#if 0	// Balance bracket
  }
#endif
#else
  extern char **environ;

  // Path to `ldd'
  char const ldd_prog[] = "/usr/bin/ldd";

  char const* argv[3];
  argv[0] = "ldd";
  argv[1] = fullpath.data();
  argv[2] = NULL;
  exec_prog(ldd_prog, argv, environ, decode);

  for(vector<my_link_map>::iterator iter = shared_libs.begin(); iter != shared_libs.end(); ++iter)
  {
    my_link_map* l = &(*iter);
#endif
    if (l->l_addr)
    {
      Dout(dc::bfd|continued_cf, "Loading debug symbols from " << l->l_name << " (" << hex << l->l_addr << ") ... ");
      object_file_ct* object_file = new object_file_ct(l->l_name, reinterpret_cast<void*>(l->l_addr));
#ifdef DEBUG
      if (object_file->get_number_of_symbols() > 0)
	Dout(dc::finish, "done (" << dec << object_file->get_number_of_symbols() << " symbols)");
      else
	Dout(dc::finish, "No symbols found");
#endif
    }
  }

  object_files().sort(object_file_greater());

  initialized = true;

#ifdef DEBUGDEBUGBFD
  // Dump all symbols
  cout << setiosflags(ios::left) << setw(15) << "Start address" << setw(50) << "BFD name" << setw(20) << "Number of symbols" << endl;
  for (object_files_ct::reverse_iterator i(object_files().rbegin()); i != object_files().rend(); ++i)
  {
    cout << "0x" << setfill('0') << setiosflags(ios::right) << setw(8) << hex << (unsigned long)(*i)->get_lbase() << "     ";
    cout << setfill(' ') << setiosflags(ios::left) << setw(50) << (*i)->get_bfd()->filename;
    cout << dec << setiosflags(ios::left);
    cout << (*i)->get_number_of_symbols() << endl;

    cout << setiosflags(ios::left) << setw(12) << "Start";
    cout << ' ' << setiosflags(ios::right) << setw(6) << "Size" << ' ';
    cout << "Name value flags\n";
    asymbol** symbol_table = (*i)->get_symbol_table();
    for (long n = (*i)->get_number_of_symbols() - 1; n > 0; --n)
    {
      cout << setiosflags(ios::left) << hex << setw(12) << symbol_start_addr(symbol_table[n]);
      cout << ' ' << setiosflags(ios::right) << setw(6) << symbol_size(symbol_table[n]) << ' ';
      cout << symbol_table[n]->name << ' ' << hex <<	symbol_table[n]->value << ' ' << oct << symbol_table[n]->flags << endl;
    }
  }
#endif

  return 0;
}

static symbol_ct const* libcw_bfd_pc_symbol(bfd_vma addr, object_file_ct* object_file)
{
  static asymbol dummy_symbol;
  static asection dummy_section;
  if (object_file)
  {
    // Make symbol_start_addr(&dummy_symbol) and symbol_size(&dummy_symbol) return the correct value
    bfd_asymbol_bfd(&dummy_symbol) = object_file->get_bfd();
    dummy_symbol.section = &dummy_section;	// Has dummy_section.vma == 0.  Use dummy_symbol.value to store (value + vma):
    dummy_symbol.value = reinterpret_cast<char const*>(addr) - reinterpret_cast<char const*>(object_file->get_lbase());
    symbol_size(&dummy_symbol) = 1;
    function_symbols_ct::iterator i(object_file->get_function_symbols().find(symbol_ct(&dummy_symbol, true)));
    if (i != object_file->get_function_symbols().end())
    {
      asymbol const* p = (*i).get_symbol();
      if (addr < (bfd_vma)symbol_start_addr(p) + symbol_size(p))
        return &(*i);
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

  symbol_ct const* symbol = libcw_bfd_pc_symbol((bfd_vma)addr, find_object_file(addr));

  if (!symbol)
    return NULL;

  return symbol->get_symbol()->name;
}

#ifdef DEBUG
// demangle.h doesn't include the extern "C", so we do it this way
extern "C" {
  char* cplus_demangle (char const* mangled, int options);
}
#define DMGL_PARAMS 1
#define DMGL_ANSI 2
#endif

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

  object_file_ct* object_file = find_object_file(addr);
  symbol_ct const* symbol = libcw_bfd_pc_symbol((bfd_vma)addr, object_file);
  if (symbol && symbol->is_defined())
  {
    asymbol const* p = symbol->get_symbol();
    bfd* abfd = bfd_asymbol_bfd(p);
    asection* sect = bfd_get_section(p);
    char const* file;
    ASSERT( object_file->get_bfd() == abfd );
    bfd_find_nearest_line(abfd, sect, const_cast<asymbol**>(object_file->get_symbol_table()),
	(char*)addr - (char*)object_file->get_lbase() - sect->vma, &file, &location.func, &location.line);

    if (file && location.line)	// When line is 0, it turns out that `file' is nonsense.
    {
      Debug( dc::bfd.off() );	// This get annoying pretty fast
      size_t len = strlen(file);
      char* filename = new char [len + 1];
      AllocTag(filename, "location_st::file");
      location.file = strcpy(filename, file);
      Debug( dc::bfd.on() );
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
    // Sanity check
    if (!p->name || location.line == 0)
    {
      if (p->name)
      {
	char* demangled_name = cplus_demangle(p->name, DMGL_PARAMS | DMGL_ANSI);
	Dout(dc::bfd, "Warning: Address " << hex << addr << " in section " << sect->name <<
	    " does not have a line number, perhaps the unit containing the function");
#ifdef __FreeBSD__
	Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << (demangled_name ? demangled_name : p->name) <<
	    "' wasn't compiled with CFLAGS=-ggdb");
#else
	Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << (demangled_name ? demangled_name : p->name) <<
	    "' wasn't compiled with CFLAGS=-g");
#endif
      }
      else
	Dout(dc::bfd, "Warning: Address in section " << sect->name << " does not contain a function");
    }
    else
      Dout(dc::bfd, "address " << hex << addr << dec << " corresponds to " << location);
#endif
    return location;
  }
  location.line = 0;
  if (symbol)
  {
    Debug( dc::bfd.off() );
    ostrstream os;
    os << "<undefined symbol: " << symbol->get_symbol()->name << '>' << ends;
    location.func = os.str();
    Debug( dc::bfd.on() );
  }
  else
    location.func = "<unknown function>";
  location.file = NULL;

  return location;
}

#endif // !DEBUGUSEBFD
