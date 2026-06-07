// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "cwd_dwarf.h"
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
  LIBCWD_ASSERT(!M_known);

  using namespace dwarf;

  if (!ensure_initialization(LIBCWD_TSD))
  {
    M_object_file = nullptr;
    M_func = S_pre_libcwd_initialization_c;
    M_initialization_delayed = addr;
    return;
  }

  // This should never happen because it is not possible that a thread ends up calling
  // location_ct::M_pc_location while writing writing to the final ostream now that the
  // memory allocation support was removed from libcwd.
  LIBCWD_ASSERT(!__libcwd_tsd.lock_interface_is_locked);

  ObjectFileInterface const* object_file = find_object_file(addr LIBCWD_COMMA_TSD);

  M_initialization_delayed = nullptr;
  if (!object_file)
  {
    Dout(dc::elfutils, "No object file for address " << addr);
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

  LocationLookupResult const location = symbol->lookup_location(int_addr, object_file->get_lbase());
  M_func = location.function_name();
  M_known = location.known;
  if (M_known)
    M_line = location.line;

  if (location.filepath) // Might still be true even if M_known is false (if we couldn't find a line number).
  {
    size_t len = strlen(location.filepath);
    M_filepath = lockable_auto_ptr<char, true>(new char[len + 1]); // LEAK5
    strcpy(M_filepath.get(), location.filepath);
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

location_ct::location_ct(location_ct const& prototype)
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

location_ct& location_ct::operator=(location_ct const& prototype)
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
  LIBCWD_ASSERT(M_known);
  os << M_filepath.get();
}

void location_ct::print_filename_on(std::ostream& os) const
{
  LIBCWD_ASSERT(M_known);
  os << M_filename;
}

} // namespace libcwd
