#include "cwd_sys.h"
#include "cwd_dwarf2.h"
#include "libcwd/class_location.h"
#include "libcwd/debug.h"

namespace libcwd {

char const* const location_ct::S_uninitialized_location_ct_c = "<uninitialized location_ct>";
char const* const location_ct::S_pre_ios_initialization_c = "<pre ios initialization>";
char const* const location_ct::S_pre_libcwd_initialization_c = "<pre libcwd initialization>";
char const* const location_ct::S_cleared_location_ct_c = "<cleared location ct>";

//
// location_ct::M_pc_location
//
// Find source file, (mangled) function name and line number of the address `addr'.
//
// Like `pc_function', this function looks up the symbol (function) that
// belongs to the address `addr' and stores the pointer to the name of that symbol
// in the member `M_func'.  When no symbol could be found then `M_func' is set to
// `libcwd::unknown_function_c'.
//
// If a symbol is found then this function attempts to lookup source file and line number
// nearest to the given address.  The result - if any - is put into `M_filepath' (source
// file) and `M_line' (line number), and `M_filename' is set to point to the filename
// part of `M_filepath'.
//
void location_ct::M_pc_location(void const* addr LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_ASSERT( !M_known );

  using namespace dwarf2;

  if (!ensure_initialization(LIBCWD_TSD))
  {
    M_object_file = nullptr;
    M_func = S_pre_libcwd_initialization_c;
    M_initialization_delayed = addr;
    return;
  }

#if LIBCWD_THREAD_SAFE
  if (__libcwd_tsd.pthread_lock_interface_is_locked)
  {
    // Some time, in another universe, this might really happen:
    //
    // We get here when we are writing debug output (and therefore set the lock)
    // and the streambuf of the related ostream overflows, this use basic_string
    // which uses the pool allocator which might just at that moment also run
    // out of memory and therefore call malloc(3).  And when THAT happens for the
    // first time, so the location from which that is called is not yet in the
    // location cache... then we get here.
    // We cannot obtain the object_files_instance lock now because another thread
    // might have obtained that already when calling malloc(3), doing a location
    // lookup for that and then realizing that no debug info was read yet for
    // the library that did that malloc(3) call and therefore wanting to print
    // debug output (Loading debug info from...) causing a dead-lock.
    //
    // PS We DID get here - 0.99.45, compiled with gcc 3.4.6, while running
    // threads_threads_shared from dejagnu (2006/11/08). I also had to stand
    // on one leg and wave high left.
    M_object_file = nullptr;
    M_func = S_pre_libcwd_initialization_c;	// Not really true, but this hardly ever happens in the first place.
    M_initialization_delayed = addr;
    return;
  }
#endif

  ObjectFileInterface const* object_file = find_object_file(addr LIBCWD_COMMA_TSD);

  M_initialization_delayed = nullptr;
  if (!object_file)
  {
    Dout(dc::bfd, "No object file for address " << addr);
    M_object_file = nullptr;
    M_func = unknown_function_c;
    M_unknown_pc = addr;
    return;
  }

  M_object_file = &object_file->get_object_file();

  uintptr_t int_addr = reinterpret_cast<uintptr_t>(addr);
  SymbolRangeInterface const* symbol = object_file->find_symbol(int_addr);
  if (!symbol)
  {
    M_func = unknown_function_c;
    M_unknown_pc = addr;
    return;
  }

  M_func = symbol->name();
  char const* filepath = nullptr;
  M_known = symbol->lookup_file_line(int_addr, &M_line, &filepath, object_file->get_lbase());

  if (filepath)         // Might still be true even if M_known is false (if we couldn't find a line number).
  {
    size_t len = strlen(filepath);
    M_filepath = lockable_auto_ptr<char, true>(new char [len + 1]);	// LEAK5
    strcpy(M_filepath.get(), filepath);
    char const* last_slash = strrchr(M_filepath.get(), '/');
    M_filename = last_slash ? last_slash + 1 : M_filepath.get();
  }
}

/**
 * \brief Reset this location object (frees memory).
 */
void location_ct::clear()
{
  if (M_known)
  {
    M_known = false;
    if (M_filepath.is_owner())
    {
      LIBCWD_TSD_DECLARATION;
      M_filepath.reset();
    }
  }
  M_object_file = nullptr;
  M_func = S_cleared_location_ct_c;
}

location_ct::location_ct(location_ct const &prototype)
{
  if ((M_known = prototype.M_known))
  {
    M_filepath = prototype.M_filepath;
    M_filename = prototype.M_filename;
    M_line = prototype.M_line;
  }
  else
    M_initialization_delayed = prototype.M_initialization_delayed;
  M_object_file = prototype.M_object_file;
  M_func = prototype.M_func;
}

location_ct& location_ct::operator=(location_ct const &prototype)
{
  if (this != &prototype)
  {
    clear();
    if ((M_known = prototype.M_known))
    {
      M_filepath = prototype.M_filepath;
      M_filename = prototype.M_filename;
      M_line = prototype.M_line;
    }
    else
      M_initialization_delayed = prototype.M_initialization_delayed;
    M_object_file = prototype.M_object_file;
    M_func = prototype.M_func;
  }
  return *this;
}

void location_ct::print_filepath_on(std::ostream& os) const
{
  LIBCWD_ASSERT( M_known );
  os << M_filepath.get();
}

void location_ct::print_filename_on(std::ostream& os) const
{
  LIBCWD_ASSERT( M_known );
  os << M_filename;
}

} // namespace libcwd
