// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "cwd_dwarf.h"
#include "libcwd/class_location.h"
#include "libcwd/debug.h"

namespace libcwd {

char const* const Location::s_uninitialized_location_ct = "<uninitialized Location>";
char const* const Location::s_pre_libcwd_initialization = "<pre libcwd initialization>";
char const* const Location::s_cleared_location_ct = "<cleared location ct>";

//
// Location::pc_location
//
// Find source file, (mangled) function name and line number of the address `pc'.
//
// Like `pc_function', this function looks up the symbol (function) that
// belongs to the address `pc' and stores the pointer to the name of that symbol
// in the member `function_name_'.  When no symbol could be found then `function_name_' is set to
// `libcwd::unknown_function_c'.
//
// If a symbol is found then this function attempts to lookup source file and line number
// nearest to the given address.  The result - if any - is put into `filepath_' (source
// file) and `line_' (line number), and `filename_' is set to point to the filename
// part of `filepath_'.
//
void Location::pc_location(void const* pc LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_ASSERT(!known_);

  using namespace dwarf;

  if (!ensure_initialization(LIBCWD_TSD))
  {
    object_file_name_ = nullptr;
    function_name_ = s_pre_libcwd_initialization;
    initialization_delayed_ = pc;
    return;
  }

  // This should never happen because it is not possible that a thread ends up calling
  // Location::pc_location while writing writing to the final ostream now that the
  // memory allocation support was removed from libcwd.
  LIBCWD_ASSERT(!__libcwd_tsd.lock_interface_is_locked);

  ObjectFileInterface const* object_file = find_object_file(pc LIBCWD_COMMA_TSD);

  initialization_delayed_ = nullptr;
  if (!object_file)
  {
    Dout(dc::elfutils, "No object file for address " << pc);
    object_file_name_ = nullptr;
    function_name_ = unknown_function_c;
    unknown_pc_ = pc;
    return;
  }

  object_file_name_ = &object_file->get_object_file();

  uintptr_t int_addr = reinterpret_cast<uintptr_t>(pc);
  SymbolRangeInterface const* symbol = object_file->find_symbol(int_addr);
  if (!symbol)
  {
    function_name_ = unknown_function_c;
    unknown_pc_ = pc;
    return;
  }

  LocationLookupResult const location = symbol->lookup_location(int_addr, object_file->get_lbase());
  function_name_ = location.function_name();
  known_ = location.known;
  if (known_)
    line_ = location.line;

  if (location.filepath) // Might still be true even if known_ is false (if we couldn't find a line number).
  {
    size_t len = strlen(location.filepath);
    filepath_ = lockable_auto_ptr<char, true>(new char[len + 1]); // LEAK5
    strcpy(filepath_.get(), location.filepath);
    char const* last_slash = strrchr(filepath_.get(), '/');
    filename_ = last_slash ? last_slash + 1 : filepath_.get();
  }
}

/**
 * \brief Reset this location object (frees memory).
 */
void Location::clear()
{
  if (known_)
  {
    known_ = false;
    if (filepath_.is_owner())
    {
      LIBCWD_TSD_DECLARATION;
      filepath_.reset();
    }
  }
  object_file_name_ = nullptr;
  function_name_ = s_cleared_location_ct;
}

Location::Location(Location const& prototype)
{
  if ((known_ = prototype.known_))
  {
    filepath_ = prototype.filepath_;
    filename_ = prototype.filename_;
    line_ = prototype.line_;
  }
  else
    initialization_delayed_ = prototype.initialization_delayed_;
  object_file_name_ = prototype.object_file_name_;
  function_name_ = prototype.function_name_;
}

Location& Location::operator=(Location const& prototype)
{
  if (this != &prototype)
  {
    clear();
    if ((known_ = prototype.known_))
    {
      filepath_ = prototype.filepath_;
      filename_ = prototype.filename_;
      line_ = prototype.line_;
    }
    else
      initialization_delayed_ = prototype.initialization_delayed_;
    object_file_name_ = prototype.object_file_name_;
    function_name_ = prototype.function_name_;
  }
  return *this;
}

void Location::print_filepath_on(std::ostream& os) const
{
  LIBCWD_ASSERT(known_);
  os << filepath_.get();
}

void Location::print_filename_on(std::ostream& os) const
{
  LIBCWD_ASSERT(known_);
  os << filename_;
}

} // namespace libcwd
