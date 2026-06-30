// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "cwd_dwarf.h"
#include "libcwd/Location.h"
#include "libcwd/debug.h"

namespace libcwd {

char const Location::uninitialized_location[] = "<uninitialized Location>";
char const Location::pre_libcwd_initialization[] = "<pre libcwd initialization>";
char const Location::cleared_location[] = "<cleared location ct>";

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
void Location::pc_location(void const* pc, LIBCWD_TSD_PARAM)
{
  LIBCWD_ASSERT(!known_);

  using namespace dwarf;

  if (!ensure_initialization(LIBCWD_TSD))
  {
    object_file_name_ = nullptr;
    function_name_ = pre_libcwd_initialization;
    initialization_delayed_ = pc;
    return;
  }

  // This should never happen because it is not possible that a thread ends up calling
  // Location::pc_location while writing writing to the final ostream now that the
  // memory allocation support was removed from libcwd.
  LIBCWD_ASSERT(!__libcwd_tsd.lock_interface_is_locked);

  ObjectFileInterface const* object_file = find_object_file(pc, LIBCWD_TSD);

  initialization_delayed_ = nullptr;
  if (!object_file)
  {
    __Dout(dc::elfutils, "No object file for address " << pc);
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
  {
#ifdef __clang__
    // Running CTest `location_default_argument_functions` gives, for example:
    //
    //   > $BUILDDIR/tests/location_default_argument_functions
    //   >>>>>>>>: base = 0x55bb903d5000, pc = 0x55bb903d81d9, pc - base = 0x31d9
    //
    // That offset is genuily mapped to line number 0 by clang:
    //
    //   > eu-addr2line -e $BUILDDIR/tests/location_default_argument_functions -sa 0x31d9 0x31dd
    //   0x00000000000031d9
    //   location_default_argument_functions.cpp:0:3
    //   0x00000000000031dd
    //   location_default_argument_functions.cpp:84:16
    //
    // This seems to be a bug because address 0x31d9 is part of a return statement of user code.
    // The user code for location_default_argument_functions is:
    //
    //     libcwd::Location const f_loc(&&f_call_location);
    //     int const expected_line = __LINE__ + 3;
    //     Dout(dc::notice, "Calling f() from main: " << f_loc << " (expected: " << expected_line << ")");
    //   f_call_location:
    //     return f_loc.line() == expected_line && f() ? EXIT_SUCCESS : EXIT_FAILURE;
    //
    // The disassembly gives:
    //
    //   > eu-objdump -d $BUILDDIR/tests/location_default_argument_functions | grep -B1 -A1 '31d9:'
    //       31d7:    eb 00                    jmp     0x31d9
    //       31d9:    48 8d 7d c8              lea     -0x38(%rbp),%rdi
    //       31dd:    e8 2e 02 00 00           callq   0x3410
    //
    // The Location lookup in f(), defined in support/location_default_argument_functions_cu.cpp is:
    //
    //     libcwd::Location const g_loc(&&g_call_location);
    //     expected_line = __LINE__ + 3; // Same as line below g_call_location.
    //     Dout(dc::notice, "calling g(arg_g1(), arg_g2()) from f: " << g_loc << " (expected: " << expected_line << ")");
    //   g_call_location:
    //     auto result_g = g(            // Line 50
    //             arg_g1(__LINE__),     // Line 51
    //             arg_g2(__LINE__)      // Line 52
    //         );
    //
    // This function prints:
    //
    //   >>>>>>>>: f: base = 0x55d5b809d000, pc = 0x55d5b80a0d98, pc - base = 0x3d98
    //
    //   >eu-addr2line -e $BUILDDIR/tests/location_default_argument_functions -sa 0x3d98 0x3d9d
    //   0x0000000000003d98
    //   location_default_argument_functions_cu.cpp:0:3
    //   0x0000000000003d9d
    //   location_default_argument_functions_cu.cpp:51:11
    //
    //   >eu-objdump -d $BUILDDIR/tests/location_default_argument_functions | grep -B1 -A1 '3d98:'
    //       3d96:    eb 00                    jmp     0x3d98
    //       3d98:    bf 33 00 00 00           mov     $0x33,%edi
    //       3d9d:    e8 fe ea ff ff           callq   0x28a0
    //
    // Thus, in this case we lose the resolution of the `mov     $0x33,%edi` and the next
    // instruction is the call to `arg_g1`, line 51, which is obviously done before the call to `g`.

    if (location.line == 0)
    {
      line_ = 0;
      // Add skip bytes to the address in an attempt to find the next instruction.
      for (int skip = 4; line_ == 0 && skip <= 8; ++skip)
      {
        LocationLookupResult const location_plus_skip = symbol->lookup_location(int_addr + skip, object_file->get_lbase());
        if (function_name_ != location_plus_skip.function_name() || !location_plus_skip.known)
          break;
        line_ = location_plus_skip.line;
      }
    }
    else
#endif // __clang__

    line_ = location.line;
  }

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
 * @brief Reset this location object (frees memory).
 */
void Location::clear()
{
  if (known_)
  {
    known_ = false;
    if (filepath_.is_owner())
      filepath_.reset();
  }
  object_file_name_ = nullptr;
  function_name_ = cleared_location;
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
