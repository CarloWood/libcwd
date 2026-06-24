// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "cwd_dwarf.h"
#include "dwarf_symbol_ranges.h"
#include "threadsafe/AIReadWriteMutex.h"
#include "threadsafe/threadsafe.h"
#include "libcwd/debug.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <limits>
#include <map>
#include <string>
#include <dlfcn.h>
#include <dwarf.h>
#include <elf.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>
#include <fcntl.h>
#include <link.h>
#ifdef HAVE_DLOPEN
#include <mutex>
#endif

namespace libcwd {
namespace _private_ {
extern std::atomic_bool WST_multi_threaded;
} // namespace _private_

// New debug channels.
namespace channels {
namespace dc {
/** @addtogroup group_default_dc */
/** \{ */

/** The ELFUTILS channel. */
Channel elfutils
#ifndef HIDE_FROM_DOXYGEN
    ("ELFUTILS")
#endif
        ;

/** \} */
} // namespace dc
} // namespace channels

namespace dwarf {

// Forward declarations.
class ObjectFileData;
class ObjectFile;

// Detect if this libcwd library is static or not.
#ifdef __PIC__
constexpr bool statically_linked = false;
#else
constexpr bool statically_linked = true;
#endif

// class PTLoadSegment
//
// Represents one loadable runtime segment of an ObjectFile.  Instances are
// created while holding ObjectFileRegistry::object_files_map()' write lock during
// initialization and then treated as immutable; later address lookup can read
// the segment start/end/flags and object_file_.lbase_ without a per-segment lock.
class PTLoadSegment
{
 private:
  ObjectFile const* object_file_;
  uintptr_t start_addr_;
  uintptr_t end_addr_;
  uint32_t flags_;

 public:
  PTLoadSegment(ObjectFile const* object_file, uintptr_t start_addr, uintptr_t end_addr, uint32_t flags) :
      object_file_(object_file), start_addr_(start_addr), end_addr_(end_addr), flags_(flags)
  {
  }

  ObjectFile const* object_file() const { return object_file_; }
  uintptr_t start_addr() const { return start_addr_; }
  uintptr_t end_addr() const { return end_addr_; }
  uint32_t flags() const { return flags_; }

  friend std::ostream& operator<<(std::ostream& os, PTLoadSegment const& segment)
  {
    os << '[' << std::hex << segment.start_addr_ << ", " << segment.end_addr_ << ')';
    return os;
  }
};

class ScopedTracker final
{
 private:
  bool& flag_;

 public:
  ScopedTracker(bool& flag) : flag_(flag)
  {
    // This object is not recursive.
    LIBCWD_ASSERT(!flag);
    flag = true;
  }
  ~ScopedTracker() { flag_ = false; }
};

class ScopedPreserveErrno final
{
 private:
  int saved_;

 public:
  ScopedPreserveErrno(int old_value) : saved_(old_value) { }
  ~ScopedPreserveErrno() { errno = saved_; }
};

static thread_local bool s_object_files_is_locked_ = false;

// class ObjectFileRegistry
//
// Owns the object-file registry operations, initialization state and public
// ObjectFileName payload for ObjectFileData.  The registry is populated from
// dl_iterate_phdr and stores each ObjectFile's PT_LOAD ranges in an end-address
// keyed map for later address lookup.
//
// To get access to the list you need to read and/or write lock it.
//
// To obtain a read lock, create a stack variable,
//
//   object_files_ts::rat object_files_r(object_files_map());    // Scoped read-lock for object_files_map().
//   ScopedTracker scoped_{s_object_files_is_locked_};
//
// Use object_files_r-> for read access (returns a std::map<uintptr_t, PTLoadSegment> const*).
//
// To obtain a write lock, create a stack variable,
//
//   object_files_ts::wat object_files_w(object_files_map());    // Scoped write-lock for object_files_map().
//   ScopedTracker scoped_{s_object_files_is_locked_};
//
// Use object_files_w-> for write access (returns a std::map<uintptr_t, PTLoadSegment>*).
//
class ObjectFileRegistry
{
 protected:
  // Address index for all currently discovered loadable ELF segments. The map key is the segment's one-past-the-end
  // address; the pointed-to PTLoadSegment stores the matching start address, flags, and a pointer to the ObjectFile.
  using object_files_ts = threadsafe::Unlocked<std::map<std::uintptr_t, PTLoadSegment const>,
                                               threadsafe::policy::ReadWrite<AIReadWriteMutex>>;

  static object_files_ts& object_files_map(); // Read-write lock protected end-address index of loaded PT_LOAD segments.

  libcwd::ObjectFileName object_file_name_; // Public facing data of this object file. Just contains the filename
                                            // and whether or not debug info was available for this object file or not.

  static std::atomic_bool s_object_files_initialized_; // Set when ObjectFileRegistry::register_initial_object_files was
                                                       // called, turning ensure_initialization into a no-op.

  // Construct the ObjectFileRegistry subobject for the derived ObjectFileData payload.
  // ObjectFileRegistry is not instantiated by itself; it groups registry-wide
  // static functions and state with the public object_file_name_ storage.
  ObjectFileRegistry(char const* filename) : object_file_name_(filename) { }

 public:
  // Accessor.
  libcwd::ObjectFileName const& object_file_name() const { return object_file_name_; }

  // Information passed to the cb_dl_iterate_phdr call back function.
  struct CallBackData
  {
    std::string executable_path_; // The full path to the current executable.
    // Used for targeted dlopen-driven object discovery:
    uintptr_t target_lbase_; // When non-zero, create an ObjectFile only for the DSO with this load base.
    mutable ObjectFile const* object_file_{}; // Set to the ObjectFile aggregate for target_lbase_.

    CallBackData(std::string const& executable_path, uintptr_t target_lbase = 0) :
        executable_path_(executable_path), target_lbase_(target_lbase)
    {
    }
  };

  friend bool ensure_initialization(LIBCWD_TSD_PARAM);
  static void register_initial_object_files();
  static ObjectFile const* iterate_program_headers(CallBackData const& data);
  static int cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data);
  static ObjectFile const* find_registered_object_file(uintptr_t lbase, object_files_ts::wat const& object_files_w);

  // Called from dlopen.
  static ObjectFile const* register_object_file_at_lbase(uintptr_t lbase);
  // Needed for Location.
  static ObjectFile const* find_object_file(uintptr_t addr);
};

//static
ObjectFileRegistry::object_files_ts& ObjectFileRegistry::object_files_map()
{
  static object_files_ts* map = new object_files_ts; // Intentionally leaked to avoid deinitialization order fiasco.
  return *map;
}

namespace {

// Return the best available mangled function name for func_die.
//
// func_die can be a DW_TAG_subprogram or DW_TAG_inlined_subroutine. Integrated
// attributes are used so declarations referenced through DW_AT_abstract_origin
// or DW_AT_specification still provide the original function name. The returned
// pointer is borrowed from libdw and remains valid until the owning Dwarf handle
// is closed.
char const* function_die_name(Dwarf_Die* func_die)
{
  Dwarf_Attribute attr;
  Dwarf_Attribute* name_attr = dwarf_attr_integrate(func_die, DW_AT_linkage_name, &attr);
  if (!name_attr)
    name_attr = dwarf_attr_integrate(func_die, DW_AT_MIPS_linkage_name, &attr);

  char const* name = dwarf_formstring(name_attr);
  if (!name)
    name = dwarf_diename(func_die);
  return name;
}

struct InlineScopeLookupResult
{
  char const* function_name{}; // Mangled inline function name.
  bool found{false}; // True when addr is covered by an inline scope.
};

// Inspect the DWARF scope chain for runtime address addr in cu_die using lbase as the load base.
//
// dwarf_getscopes returns scopes from innermost to outermost, so the first
// DW_TAG_inlined_subroutine is the inline body that should override the physical
// machine-code symbol for source-level reporting. The returned function_name is
// borrowed from libdw and may be null when the inline scope exists but carries no
// resolvable name. The only side effects are libdw's temporary scope allocation,
// which is released before returning, and optional debug-channel diagnostics.
InlineScopeLookupResult lookup_innermost_inline_scope(Dwarf_Die* cu_die, uintptr_t addr, uintptr_t lbase)
{
  InlineScopeLookupResult result;
  Dwarf_Die* scopes = nullptr;
  int const nscopes = dwarf_getscopes(cu_die, static_cast<Dwarf_Addr>(addr - lbase), &scopes);
  if (nscopes < 0)
    Dout(dc::elfutils, "dwarf_getscopes failed for address 0x" << std::hex << addr << ": " << dwarf_errmsg(-1));
  else
    for (int scope_index = 0; scope_index < nscopes; ++scope_index)
    {
      if (dwarf_tag(&scopes[scope_index]) != DW_TAG_inlined_subroutine)
        continue;

      result.found = true;
      result.function_name = function_die_name(&scopes[scope_index]);
      Dout(dc::elfutils, "inline scope for address 0x"
                             << std::hex << addr << " resolves to "
                             << (result.function_name ? result.function_name : "<unnamed inline function>"));
      break;
    }

  std::free(scopes);
  return result;
}

} // namespace

#ifndef HIDE_FROM_DOXYGEN
LocationLookupResult SymbolRange::lookup_location(uintptr_t addr, uintptr_t lbase) const
{
  LocationLookupResult result;
  result.physical_function_name = name_;

  Dwarf_Die cu_die;
  if (!die_.addr || dwarf_diecu(const_cast<Dwarf_Die*>(&die_), &cu_die, NULL, NULL) == nullptr)
    return result;

  InlineScopeLookupResult const inline_scope = lookup_innermost_inline_scope(&cu_die, addr, lbase);
  if (inline_scope.function_name)
    result.effective_function_name = inline_scope.function_name;

  Dwarf_Line* line = dwarf_getsrc_die(&cu_die, addr - lbase);
  if (!line)
    return result;

  bool have_lineno = false;
  int lineno;
  if (dwarf_lineno(line, &lineno) != 0)
    Dout(dc::elfutils, "dwarf_lineno failed for address " << addr << ": " << dwarf_errmsg(-1));
  else
  {
    have_lineno = true;
    result.line = lineno;
  }

  if ((result.filepath = dwarf_linesrc(line, nullptr, nullptr)) == nullptr)
  {
    Dout(dc::elfutils, "dwarf_linesrc failed for address 0x" << std::hex << addr << ": " << dwarf_errmsg(-1));
    return result;
  }

  result.known = have_lineno;
  return result;
}
#endif // HIDE_FROM_DOXYGEN

// class ObjectFileData
//
// Represents a single object file: either the executable or a shared library.
// Registry-wide operations live in ObjectFileRegistry; instances only own the
// per-object load base and libdw state.
class ObjectFileData : public ObjectFileRegistry
{
 public:
  static constexpr int dwarf_symbols_loaded_not_called = -2;

 private:
  int dwarf_fd_{
      dwarf_symbols_loaded_not_called}; // Once load_dwarf_symbols was called this becomes -1 (permanent failure) or
                                        // equal to the open fd for the file containing the debug info.
  Dwarf* dwarf_handle_{nullptr}; // mutable because close_dwarf() sets these to nullptr and -1 again.
  bool elf_symbols_loaded_{false}; // True iff load_elf_function_symbols was already called.

  using function_symbols_type = FunctionSymbolRanges;
  function_symbols_type function_symbols_; // End-address index of SymbolRange's.

 public:
  // Accessors.
  bool is_initialized() const { return dwarf_fd_ >= 0; }
  bool dwarf_symbols_loaded() const { return dwarf_fd_ != dwarf_symbols_loaded_not_called; }
  bool elf_symbols_loaded() const { return elf_symbols_loaded_; }

  // Construct and destroy the protected ObjectFile payload inside object_file_data_ts.
  // Construction records the path/load base only; symbol loading is explicitly
  // triggered later through ObjectFile::load_dwarf_symbols(), which serializes the
  // mutable DWARF initialization behind this wrapper's read/write lock.
  // Destruction releases any libdw resources owned by the payload.
  ObjectFileData(char const* filename, uintptr_t lbase);
  ~ObjectFileData();

  // Load libdw state for this object file from debug_info_path which must be computed
  // without holding a lock because libdwfl may call dlopen while resolving separate debug
  // info.
  //
  // Note that dwarf_symbols_loaded() becomes true even when opening fails so concurrent callers
  // know that initialization has already been attempted and can continue without retrying.
  void load_dwarf_symbols(uintptr_t lbase, std::string const& debug_info_path);

  // Load ELF symbols for fallback.
  void load_elf_function_symbols(uintptr_t lbase, std::string const& object_file_path);

  // Look up a SymbolRange in function_symbols_ by address.
  // Never call this member function directly. Use ObjectFile::find_symbol instead.
  SymbolRange const* get_symbol_range_from_function_symbols_map(uintptr_t addr) const;

 private:
  struct LoadFunctionSymbolRangesContext;

  void open_dwarf(uintptr_t lbase, std::string const& debug_info_path);
  void load_function_symbols(uintptr_t lbase);
  void add_elf_function_symbol(GElf_Sym const& sym, char const* name, uintptr_t lbase);
  static int cb_load_function_symbol(Dwarf_Die* func_die, void* arg);
  void add_function_symbol(Dwarf_Die* func_die, uintptr_t lbase);
  bool add_function_symbol_range(Dwarf_Addr start_pc, Dwarf_Addr end_pc, char const* name, Dwarf_Die const* func_die,
                                 uintptr_t lbase);
  static char const* function_symbol_name(Dwarf_Die* func_die);
  void close_dwarf();

 public:
  // Called from dlclose.
  ObjectFile const* unregister_object_file_ranges(ObjectFile const* self);
};

class ObjectFile final : public ObjectFileInterface
{
 private:
  using object_file_data_ts = threadsafe::Unlocked<ObjectFileData, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  mutable object_file_data_ts object_file_data_; // Thread-safe storage wrapper for ObjectFile instances.

 public:
  ObjectFile(uintptr_t lbase, char const* filename) : ObjectFileInterface(lbase), object_file_data_(filename, lbase) { }

  ObjectFile(ObjectFile const&) = delete;
  ObjectFile& operator=(ObjectFile const&) = delete;

  // Ensure that the symbols of this ObjectFile have been loaded.
  void realize_symbols() const;

  // Only use this to construct a wat or rat object (used in dlclose)!
  object_file_data_ts& unlocked_data() const { return object_file_data_; }

  ObjectFileName const& get_object_file() const override
  {
    object_file_data_ts::rat data_r(object_file_data_);
    return data_r->object_file_name();
  }

  SymbolRangeInterface const* find_symbol(uintptr_t addr) const override;

  // Called from dlopen.
  object_file_data_ts::wat write_locked_data() const { return object_file_data_ts::wat{object_file_data_}; }
};

//static
std::atomic_bool ObjectFileRegistry::s_object_files_initialized_ = false;

namespace {

// Return the ELF file that should be opened for DWARF data for object_file.
// Downloads of missing debug info by debuginfod can happen if DEBUGINFOD_URLS is set,
// otherwise libdwfl can still use already installed local debuginfo and ordinary
// debuglink paths.
//
// The returned path is either the separate debuginfo file selected by libdwfl or
// object_file itself when no separate file was found; the caller should open the
// path with libdw which reports missing/no-DWARF cases.
//
// Note that we must return an owning std::string because the `char const*`s returned
// by dwfl_module_info are no longer valid after `close_dwfl` is destroyed.
std::string get_debug_info_path(char const* object_file_path)
{
  ScopedPreserveErrno scoped_(errno);

  Dwfl_Callbacks callbacks{};
  callbacks.find_debuginfo = dwfl_standard_find_debuginfo;
  callbacks.section_address = dwfl_offline_section_address;

  Dwfl* dwfl = dwfl_begin(&callbacks);
  if (!dwfl)
  {
    Dout(dc::warning,
         "dwfl_begin failed while looking for debuginfo of \"" << object_file_path << "\": " << dwfl_errmsg(-1));
    return object_file_path;
  }

  struct DwflCloser
  {
    Dwfl* dwfl_;
    ~DwflCloser() { dwfl_end(dwfl_); }
  } close_dwfl{dwfl};

  Dwfl_Module* module = dwfl_report_offline(dwfl, object_file_path, object_file_path, -1);
  if (!module)
  {
    Dout(dc::warning, "dwfl_report_offline failed for \"" << object_file_path << "\": " << dwfl_errmsg(-1));
    return object_file_path;
  }

  if (dwfl_report_end(dwfl, nullptr, nullptr) != 0)
  {
    Dout(dc::warning, "dwfl_report_end failed for \"" << object_file_path << "\": " << dwfl_errmsg(-1));
    return object_file_path;
  }

  // Force libdwfl to locate and validate the separate debuginfo file now; only
  // after this call does dwfl_module_info reliably expose the selected debugfile.
  Dwarf_Addr bias;
  (void)dwfl_module_getdwarf(module, &bias);

  char const* mainfile = nullptr;
  char const* debugfile = nullptr;
  dwfl_module_info(module, nullptr, nullptr, nullptr, nullptr, nullptr, &mainfile, &debugfile);

  if (debugfile && debugfile[0] != '\0')
    return debugfile;
  if (mainfile && mainfile[0] != '\0')
    return mainfile;
  return object_file_path;
}

// Find the full path to the current running process.
// This needs to work before we reach main, therefore it uses the /proc filesystem.
std::string current_executable_path()
{
  std::string result(256, '\0');

  for (;;)
  {
    ssize_t const n = ::readlink("/proc/self/exe", result.data(), result.size());

    if (n < 0)
      throw std::runtime_error(std::string("readlink(/proc/self/exe) failed: ") + std::strerror(errno));

    if (static_cast<size_t>(n) < result.size())
    {
      result.resize(static_cast<size_t>(n));
      return result;
    }

    result.resize(result.size() * 2);
  }
}

// Return a compact textual representation of ELF program-header permission flags.
// The result is used only for diagnostics while building the dwarf object/segment cache.
// Output character positions correspond to respectively PF_R, PF_W, PF_X; a dash is
// printed if that permission bit is absent.
char const* flags_to_string(ElfW(Word) flags)
{
  static thread_local char result[4];
  result[0] = (flags & PF_R) ? 'R' : '-';
  result[1] = (flags & PF_W) ? 'W' : '-';
  result[2] = (flags & PF_X) ? 'X' : '-';
  result[3] = '\0';
  return result;
}

struct ForceLoadingDebugOutput
{
  // LIBCWD_NO_STARTUP_MSGS deliberately suppresses even the loading trace that
  // LIBCWD_PRINT_LOADING would otherwise force on.
  //
  // Do we need debug output regarding the loading of object files and their symbols?
  bool const forced_loading_output{_private_::always_print_loading && !_private_::suppress_startup_msgs};

  libcwd::DebugObject::OnOffState do_state;
  libcwd::Channel::OnOffState elfutils_state;

  ForceLoadingDebugOutput()
  {
    if (forced_loading_output)
    {
      Debug(libcw_do.force_on(do_state));
      Debug(dc::elfutils.force_on(elfutils_state, "ELFUTILS"));
    }
  }

  ~ForceLoadingDebugOutput()
  {
    if (forced_loading_output)
    {
      Debug(dc::elfutils.restore(elfutils_state));
      Debug(libcw_do.restore(do_state));
    }
  }
};

} // namespace

void ObjectFile::realize_symbols() const
{
  char const* object_file_path;
  {
    // Read-lock the object_file_data_ts.
    object_file_data_ts::rat data_r(object_file_data_);

    // Return if already loaded.
    if (data_r->dwarf_symbols_loaded())
      return;

    object_file_path = data_r->object_file_name().filepath();
  }

  // This lock may not be held (by this thread).
  LIBCWD_ASSERT(!s_object_files_is_locked_);

  // Do not call get_debug_info_path before main has been reached.
  // Through libdebuginfod/libcurl it may re-enter loader/runtime initialization
  // while global constructors are still running, which can deadlock.
  if (!test_main_reached())
    return;

  // Obtain the debug info path without holding the lock on *data_ because
  // this call can cause a recursive call to dlopen.
  std::string debug_info_path = get_debug_info_path(object_file_path);

  for (;;)
  {
    try
    {
      object_file_data_ts::rat data_r(object_file_data_);

      if (data_r->dwarf_symbols_loaded())
        return; // Someone else beat us to it.

      object_file_data_ts::wat data_w(data_r);
      data_w->load_dwarf_symbols(lbase_, debug_info_path);
    }
    catch (std::exception const&)
    {
      object_file_data_.rd2wryield();
      continue;
    }
    break;
  }
}

// On the lifetime of the returned object.
//
// There are only two callers of this function, `libcwd::Location::pc_location` and
// `libcwd::pc_mangled_function_name`. Both first call
//     object_file = find_object_file(addr)
//     symbol = object_file->find_symbol(addr)
// and then either read `symbol->name()` or let `symbol->lookup_location(...)`
// select an inline-effective source name and file-line location.
//
// If main_reached() wasn't called yet then we should be single-threaded and the access to `name()` is safe.
// The call to `lookup_location` is a no-op in this case.
//
// If main_reached() *was* called then the call to `find_object_file` that called `object_file->realize_symbols()`
// will have assured a call to `load_dwarf_symbols` and `load_function_symbols` or at the very least, in the
// case of an error, that that will never be called again for that object file.
//
// Therefore any SymbolRange added to function_symbols_ should be stable (unless the corresponding DSO is dlclose-d)
// because the only call to erase for elements of function_symbols_ happens in `insert_function_symbol_range`,
// which can only be called indirectly from `load_function_symbols` (or `load_elf_function_symbols` but then
// such a DWARF SymbolRange can't be invalidated because "DWARF wins from ELF ranges") and that was already called.
//
SymbolRangeInterface const* ObjectFile::find_symbol(uintptr_t addr) const
{
  SymbolRangeInterface const* symbol;
  for (;;)
  {
    try
    {
      object_file_data_ts::rat data_r(object_file_data_);
      symbol = data_r->get_symbol_range_from_function_symbols_map(addr);
      if (!symbol && !data_r->elf_symbols_loaded())
      {
        {
          object_file_data_ts::wat data_w(data_r);
          data_w->load_elf_function_symbols(lbase_, data_r->object_file_name().filepath());
        }
        symbol = data_r->get_symbol_range_from_function_symbols_map(addr);
      }
    }
    catch (std::exception const&)
    {
      object_file_data_.rd2wryield();
      continue;
    }
    break;
  }
  return symbol;
}

SymbolRange const* ObjectFileData::get_symbol_range_from_function_symbols_map(uintptr_t addr) const
{
  SymbolRange const* symbol = nullptr;
  auto const iter = function_symbols_.upper_bound(addr);
  if (iter != function_symbols_.end() && iter->second.start_addr() <= addr)
    symbol = &iter->second;
  return symbol;
}

//static
void ObjectFileRegistry::register_initial_object_files()
{
  ForceLoadingDebugOutput scoped_;

  // Initialize object files list, we don't really need the
  // write lock because this function is Single Threaded.
  //
  // Start a new scope for the write lock.
  {
    CallBackData const data{current_executable_path()};

    // This is the initial population pass for the dwarf object/segment cache.
    // Even if dlopen is called first, that still will first call this function.
    LIBCWD_ASSERT(object_files_ts::rat{object_files_map()}->empty());

    // Load executable and shared objects.
    if (!statically_linked)
    {
      // Iterate over all currently loaded object files and create new ObjectFile's for them.
      iterate_program_headers(data);
    }

    // Relaxed is OK: we get here from the main thread before any other thread (that
    // might come here without other synchronization mechanism) has been created yet.
    s_object_files_initialized_.store(true, std::memory_order_relaxed);
  } // Unlock object_files_map().
}

//static
ObjectFile const* ObjectFileRegistry::iterate_program_headers(CallBackData const& data)
{
  // From https://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html:
  //
  // The dl_iterate_phdr() function walks through the list of an application's shared objects and
  // calls the function cb_dl_iterate_phdr once for each object, until either all shared objects
  // have been processed or cb_dl_iterate_phdr returns a nonzero value.
  //
  // A non-zero data.target_lbase_ makes the callback stop after the matching object;
  // zero means enumerate all currently loaded objects.
  //
  dl_iterate_phdr(cb_dl_iterate_phdr, const_cast<CallBackData*>(&data));
  return data.object_file_;
}

//static
int ObjectFileRegistry::cb_dl_iterate_phdr(dl_phdr_info* info, size_t /*UNUSED*/, void* call_back_data)
{
  CallBackData const* data = static_cast<CallBackData const*>(call_back_data);
  uintptr_t const lbase = static_cast<uintptr_t>(info->dlpi_addr);
  bool const target_lbase_only = data->target_lbase_ != 0;

  // If this is not the target object; advance to the next.
  if (target_lbase_only && lbase != data->target_lbase_)
    return 0;

  // dl_iterate_phdr reports an empty name for the main executable.
  // Use the already resolved /proc/self/exe path in that case so ObjectFileName remains
  // useful for diagnostics and later public location data.
  char const* object_filename =
      (info->dlpi_name && info->dlpi_name[0] != '\0') ? info->dlpi_name : data->executable_path_.c_str();

  // A DSO can be discovered through a delayed dlopen registration while an
  // outer dl_iterate_phdr pass is still running.  Avoid constructing and
  // inserting a duplicate ObjectFile when the iterator later reaches the same
  // link-map entry.
  {
    object_files_ts::wat object_files_w(object_files_map());
    ScopedTracker scoped_{s_object_files_is_locked_};
    if (ObjectFile const* existing_object_file = find_registered_object_file(lbase, object_files_w))
    {
      data->object_file_ = existing_object_file;
      return target_lbase_only;
    }
  }

  // Create a new ObjectFileData wrapper.  The wrapper address, not the protected
  // ObjectFile payload address, is stored in every PTLoadSegment so all future
  // users are forced through threadsafe access objects before touching mutable
  // per-object DWARF state.
  ObjectFile const* object_file = new ObjectFile(lbase, object_filename);

  Dout(dc::elfutils, "Adding PT_LOAD segments for \"" << object_filename << "\" at 0x" << std::hex << lbase
                                                      << " pointing to ObjectFile at " << object_file);

  // Insert one immutable PTLoadSegment per loadable segment into the end-keyed map.
  // The map owns the discoverable active address ranges; the pointed-to objects are intentionally kept stable
  // for the process lifetime until later dlclose/tombstone semantics are designed.
  bool have_pt_load_segment = false;
  for (ElfW(Half) phdr_index = 0; phdr_index < info->dlpi_phnum; ++phdr_index)
  {
    ElfW(Phdr) const& phdr = info->dlpi_phdr[phdr_index];
    if (phdr.p_type != PT_LOAD ||
        // The Elf-64 Object File Format specifies that p_memsz may be zero
        // (https://man7.org/linux/man-pages/man5/elf.5.html)
        phdr.p_memsz == 0)
      continue;

    uintptr_t const segment_start = static_cast<uintptr_t>(info->dlpi_addr + phdr.p_vaddr);
    uintptr_t const segment_end = segment_start + static_cast<uintptr_t>(phdr.p_memsz);
    LIBCWD_ASSERT(segment_end > segment_start);

    object_files_ts::wat object_files_w(object_files_map());
    ScopedTracker scoped_{s_object_files_is_locked_};
    auto const ibp = object_files_w->try_emplace(segment_end, object_file, segment_start, segment_end, phdr.p_flags);
    // Make sure the new segment doesn't overlap with an already existing one.
    LIBCWD_ASSERT(
        ibp.second && (ibp.first == object_files_w->begin() || std::prev(ibp.first)->first <= segment_start) &&
        (std::next(ibp.first) == object_files_w->end() || std::next(ibp.first)->second.start_addr() >= segment_end));

    Dout(dc::elfutils, "    map[" << (void*)segment_end << "] = PTLoadSegment " << ibp.first->second << " ["
                                  << (void*)segment_start << '-' << (void*)segment_end
                                  << ") flags=" << flags_to_string(phdr.p_flags));

    // Set data->object_file_ to point to the ObjectFile member of the first PTLoadSegment element of
    // object_files_map().
    have_pt_load_segment = true;
  }

  // If no PTLoadSegment was inserted, then we want to ignore this object file.
  if (have_pt_load_segment)
    data->object_file_ = object_file;
  else
    delete object_file;

  // Stop iterating iff there was only one target.
  return target_lbase_only;
}

//static
ObjectFile const* ObjectFileRegistry::find_registered_object_file(uintptr_t lbase,
                                                                  object_files_ts::wat const& object_files_w)
{
  // The writable map is keyed by PT_LOAD end address and can contain multiple
  // segments for the same object file.  Treat any segment whose owning
  // ObjectFile has the requested load base as proof that the object was already
  // registered; this prevents duplicate ObjectFile creation when loader
  // enumeration reports a DSO that was discovered through an earlier path.
  for (auto const& [segment_end, segment] : *object_files_w)
  {
    if (segment.object_file()->get_lbase() == lbase)
      return segment.object_file();
  }
  return nullptr;
}

//static
ObjectFile const* ObjectFileRegistry::register_object_file_at_lbase(uintptr_t lbase)
{
  // Locks object_files_map().
  CallBackData const data{current_executable_path(), lbase};

  // If this object file was already loaded before, then return that instead of creating a new ObjectFile.
  ObjectFile const* existing_object_file = find_registered_object_file(lbase, object_files_ts::wat{object_files_map()});

  // Otherwise iterate over all currently loaded object files to find the just dynamically
  // opened object file, loaded at data.target_lbase_, and create a new ObjectFile for it.
  return existing_object_file ? existing_object_file : iterate_program_headers(data);
}

//static
ObjectFile const* ObjectFileRegistry::find_object_file(uintptr_t addr)
{
  // The object file found.
  ObjectFile const* object_file = nullptr;

  {
    object_files_ts::rat object_files_r(object_files_map());
    ScopedTracker scoped_{s_object_files_is_locked_};
    auto const iter = object_files_r->upper_bound(addr);
    if (iter != object_files_r->end() && iter->second.start_addr() <= addr)
      object_file = iter->second.object_file();
  }

  // realize_symbols must be called without holding the lock on object_files_map().
  if (object_file)
    object_file->realize_symbols();

  return object_file;
}

ObjectFileInterface const* find_object_file(void const* addr, LIBCWD_TSD_PARAM)
{
  if (!ensure_initialization(LIBCWD_TSD))
    return nullptr;

  return ObjectFileRegistry::find_object_file(reinterpret_cast<uintptr_t>(addr));
}

// Free function.
bool ensure_initialization(LIBCWD_TSD_PARAM)
{
  if (ObjectFileRegistry::s_object_files_initialized_.load(std::memory_order_relaxed))
    return true;

  static bool WST_being_initialized = false;
  // Detect recursive initialization.
  if (WST_being_initialized)
    return false;
  WST_being_initialized = true;

  // This must be called before calling register_initial_object_files.
  if (!libcw_do.NS_init(LIBCWD_TSD))
    return false;

  // MT: We assume this is called before reaching main().
  //     Therefore, no synchronisation is required.
#if CWDEBUG_DEBUG
  if (_private_::WST_multi_threaded.load(std::memory_order_relaxed))
    core_dump();
#endif

  ObjectFileRegistry::register_initial_object_files();

  return true;
}

ObjectFileData::ObjectFileData(char const* filename, uintptr_t lbase) : ObjectFileRegistry(filename)
{
  Dout(dc::elfutils, "new ObjectFile \"" << filename << "\" with load base 0x" << std::hex << lbase);
}

ObjectFileData::~ObjectFileData()
{
  Dout(dc::elfutils, "destroying ObjectFile \"" << object_file_name_.filepath() << "\".");
  close_dwarf();
}

ObjectFile const* ObjectFileData::unregister_object_file_ranges(ObjectFile const* self)
{
  ObjectFile const* obsolete_object_file = nullptr;
  // Remove all PTLoadSegment's from object_files_map() that belong to this ObjectFile.
  // `self` is the stable wrapper pointer stored in each PTLoadSegment; compare
  // that pointer directly so this write-locked ObjectFile does not need to take
  // a nested read lock on itself while erasing its ranges.
  {
    object_files_ts::wat object_files_w(object_files_map());
    ScopedTracker scoped_{s_object_files_is_locked_};
    for (auto iter = object_files_w->begin(); iter != object_files_w->end();)
    {
      PTLoadSegment const& segment = iter->second;
      if (segment.object_file() == self)
      {
        Dout(dc::elfutils, "removing map[" << (void*)iter->first << "] = PTLoadSegment " << segment << " for object "
                                           << this << " (\"" << object_file_name_.filename() << "\")");
        obsolete_object_file = segment.object_file(); // Every segment points to the same ObjectFile.
        iter = object_files_w->erase(iter);
      }
      else
        ++iter;
    }
  }
  // Free libdw resources.
  close_dwarf();
  // Return the ObjectFile that is now no longer in use.
  return obsolete_object_file;
}

void ObjectFileData::load_dwarf_symbols(uintptr_t lbase, std::string const& debug_info_path)
{
  open_dwarf(lbase, debug_info_path);
  if (is_initialized())
    load_function_symbols(lbase);
}

// struct ObjectFileData::LoadFunctionSymbolRangesContext
//
// Carries the object-file cache being populated and its runtime load base through
// libdw's C callback API.  The callback stores discovered function DIE ranges in
// function_symbols_; symbol addresses are converted from DWARF/link-time PC
// values to runtime addresses by adding lbase_ before insertion.
struct ObjectFileData::LoadFunctionSymbolRangesContext
{
  ObjectFileData* object_file_data_;
  uintptr_t lbase_;
  std::size_t symbol_count_{};

  LoadFunctionSymbolRangesContext(ObjectFileData* object_file_data, uintptr_t lbase) :
      object_file_data_(object_file_data), lbase_(lbase)
  {
  }
};

// ObjectFileData::load_function_symbols
//
// Walk every compilation unit in the opened DWARF handle and ask libdw to visit
// each defining DW_TAG_subprogram.  For every function DIE the callback records
// one SymbolRange per contiguous PC range.  The strings returned by libdw are not
// copied; they remain valid while dwarf_handle_ is open and are discarded with
// this ObjectFileData instance.
void ObjectFileData::load_function_symbols(uintptr_t lbase)
{
  LIBCWD_ASSERT(is_initialized() && dwarf_handle_);

  LoadFunctionSymbolRangesContext context{this, lbase};
  Dwarf_Off offset = 0;
  Dwarf_Off next_offset = 0;
  size_t header_size = 0;
  int nextcu_status;

  while ((nextcu_status = dwarf_nextcu(dwarf_handle_, offset, &next_offset, &header_size, nullptr, nullptr, nullptr)) ==
         0)
  {
    Dwarf_Die cu_die_mem;
    Dwarf_Die* cu_die = dwarf_offdie(dwarf_handle_, offset + header_size, &cu_die_mem);
    if (!cu_die)
      Dout(dc::elfutils, "dwarf_offdie failed for CU at offset " << offset << " in " << object_file_name_.filepath()
                                                                 << ": " << dwarf_errmsg(-1));
    else if (dwarf_getfuncs(cu_die, &ObjectFileData::cb_load_function_symbol, &context, 0) < 0)
      Dout(dc::elfutils, "dwarf_getfuncs failed for CU at offset " << offset << " in " << object_file_name_.filepath()
                                                                   << ": " << dwarf_errmsg(-1));

    offset = next_offset;
  }

  if (nextcu_status < 0)
    Dout(dc::elfutils,
         "dwarf_nextcu failed while loading symbols from " << object_file_name_.filepath() << ": " << dwarf_errmsg(-1));

  Dout(dc::elfutils,
       "Loaded " << context.symbol_count_ << " function symbol range(s) from " << object_file_name_.filepath());
}

// ObjectFileData::cb_load_function_symbol
//
// libdw callback for dwarf_getfuncs.  It delegates the DIE interpretation to the
// owning ObjectFileData and keeps traversal going so all defining subprograms in
// the compilation unit are cached.
//
//static
int ObjectFileData::cb_load_function_symbol(Dwarf_Die* func_die, void* arg)
{
  LoadFunctionSymbolRangesContext* context = static_cast<LoadFunctionSymbolRangesContext*>(arg);
  std::size_t const before = context->object_file_data_->function_symbols_.size();
  context->object_file_data_->add_function_symbol(func_die, context->lbase_);
  context->symbol_count_ += context->object_file_data_->function_symbols_.size() - before;
  return DWARF_CB_OK;
}

// ObjectFileData::add_function_symbol
//
// Extract the best available function name and all address ranges from a DW_TAG_subprogram DIE.
void ObjectFileData::add_function_symbol(Dwarf_Die* func_die, uintptr_t lbase)
{
  char const* name = function_symbol_name(func_die);
  if (!name)
    return;

  Dwarf_Addr base = 0;
  Dwarf_Addr start_pc = 0;
  Dwarf_Addr end_pc = 0;
  for (ptrdiff_t range_offset = 0;
       (range_offset = dwarf_ranges(func_die, range_offset, &base, &start_pc, &end_pc)) != 0;)
  {
    if (range_offset == -1)
    {
      Dout(dc::elfutils, "dwarf_ranges failed for function " << name << " in " << object_file_name_.filepath() << ": "
                                                             << dwarf_errmsg(-1));
      break;
    }

    add_function_symbol_range(start_pc, end_pc, name, func_die, lbase);
  }
}

// ObjectFileData::add_function_symbol_range
//
// Insert one contiguous function range into function_symbols_.
//
// DWARF reports one-past-the-end high_pc values; the map key is kept equal to SymbolRange::end_addr() so find_symbol
// can use upper_bound(addr). Empty, inverted, or overflowing ranges are ignored because they cannot safely identify a
// runtime PC. Overlaps are resolved by insert_function_symbol_range. Returns true when at least one new fragment was
// inserted.
bool ObjectFileData::add_function_symbol_range(Dwarf_Addr start_pc, Dwarf_Addr end_pc, char const* name,
                                               Dwarf_Die const* func_die, uintptr_t lbase)
{
  if (!(start_pc < end_pc && end_pc <= std::numeric_limits<uintptr_t>::max() - lbase))
    return false;

  uintptr_t const start_addr = lbase + static_cast<uintptr_t>(start_pc);
  uintptr_t const end_addr = lbase + static_cast<uintptr_t>(end_pc);

  return insert_function_symbol_range(function_symbols_, start_addr, end_addr, name, func_die);
}

// ObjectFileData::load_elf_function_symbols
//
// Populate function_symbols_ from the ELF symbol tables of object_file_path.
// This is the fallback used when an object has no DWARF debug information: the
// regular .symtab/.dynsym function entries still provide address ranges and
// names such as `main', but no source line DIE is available.  Names copied from
// libelf string tables are intentionally leaked because SymbolRange stores raw stable
// pointers and the Elf handle is closed before location lookups use the cache.
void ObjectFileData::load_elf_function_symbols(uintptr_t lbase, std::string const& object_file_path)
{
  // Only attempt this once.
  elf_symbols_loaded_ = true;

  if (elf_version(EV_CURRENT) == EV_NONE)
  {
    Dout(dc::elfutils, "elf_version failed while loading symbols from " << object_file_path << ": " << elf_errmsg(-1));
    return;
  }

  int fd = open(object_file_path.c_str(), O_RDONLY);
  if (fd == -1)
  {
    Dout(dc::elfutils, "failed to open ELF file " << object_file_path << " while loading fallback symbols");
    return;
  }

  Elf* elf = elf_begin(fd, ELF_C_READ, nullptr);
  if (!elf)
  {
    Dout(dc::elfutils, "elf_begin failed for " << object_file_path << ": " << elf_errmsg(-1));
    close(fd);
    return;
  }

  std::size_t symbol_count = 0;
  Elf_Scn* section = nullptr;
  while ((section = elf_nextscn(elf, section)) != nullptr)
  {
    GElf_Shdr section_header;
    if (!gelf_getshdr(section, &section_header))
      continue;

    if (section_header.sh_type != SHT_SYMTAB && section_header.sh_type != SHT_DYNSYM)
      continue;

    Elf_Data* data = nullptr;
    while ((data = elf_getdata(section, data)) != nullptr)
    {
      if (section_header.sh_entsize == 0)
        continue;
      std::size_t const entries = data->d_size / section_header.sh_entsize;
      for (std::size_t index = 0; index < entries; ++index)
      {
        GElf_Sym sym;
        if (!gelf_getsym(data, static_cast<int>(index), &sym))
          continue;

        char const* name = elf_strptr(elf, section_header.sh_link, sym.st_name);
        std::size_t const before = function_symbols_.size();
        add_elf_function_symbol(sym, name, lbase);
        symbol_count += function_symbols_.size() - before;
      }
    }
  }

  elf_end(elf);
  close(fd);

  Dout(dc::elfutils, "Loaded " << symbol_count << " ELF function symbol range(s) from " << object_file_path);
}

// ObjectFileData::add_elf_function_symbol
//
// Add one STT_FUNC (or GNU ifunc) ELF symbol as a function range.  ELF symbol
// table addresses are link-time virtual addresses, so lbase converts them to the
// same runtime address space that find_symbol receives.  Zero-sized or undefined
// symbols cannot safely answer containment queries and are skipped.
void ObjectFileData::add_elf_function_symbol(GElf_Sym const& sym, char const* name, uintptr_t lbase)
{
  if (!name || name[0] == '\0' || sym.st_shndx == SHN_UNDEF || sym.st_size == 0)
    return;

  unsigned char const type = GELF_ST_TYPE(sym.st_info);
  if (type != STT_FUNC
#ifdef STT_GNU_IFUNC
      && type != STT_GNU_IFUNC
#endif
  )
    return;

  if (sym.st_value > std::numeric_limits<Dwarf_Addr>::max() - sym.st_size ||
      sym.st_value > std::numeric_limits<uintptr_t>::max() - lbase ||
      sym.st_value + sym.st_size > std::numeric_limits<uintptr_t>::max() - lbase)
    return;

  char* stable_name = static_cast<char*>(std::malloc(std::strlen(name) + 1));
  if (!stable_name)
    return;
  std::strcpy(stable_name, name);

  if (!add_function_symbol_range(static_cast<Dwarf_Addr>(sym.st_value),
                                 static_cast<Dwarf_Addr>(sym.st_value + sym.st_size), stable_name, nullptr, lbase))
    std::free(stable_name);
}

// ObjectFileData::function_symbol_name
//
// Return a stable libdw-owned string for the given function DIE.
// Linkage-name attributes are checked with dwarf_attr_integrate so declarations referenced
// via DW_AT_abstract_origin or DW_AT_specification still provide the mangled name;
// the returned pointer is valid until the owning Dwarf handle is closed.
//
// The standard DW_AT_linkage_name attribute is preferred because it contains the mangled
// linker symbol used by existing Location callers; the GNU/MIPS spelling and DW_AT_name
// are fallbacks for older or non-C++ producer output.
//
//static
char const* ObjectFileData::function_symbol_name(Dwarf_Die* func_die)
{
  return function_die_name(func_die);
}

void ObjectFileData::open_dwarf(uintptr_t lbase, std::string const& debug_info_path)
{
  ScopedPreserveErrno scoped_(errno);

  // Should only be called once, through load_dwarf_symbols() while holding the write lock.
  LIBCWD_ASSERT(!dwarf_symbols_loaded());

  bool different_symbols_path = debug_info_path != object_file_name_.filepath();
  Dout(dc::elfutils | continued_cf,
       "Loading debug info " << (different_symbols_path ? "for " : "from ") << object_file_name_.filepath());
  if (different_symbols_path)
    Dout(dc::continued, " (from " << debug_info_path << ")");
  Dout(dc::continued | flush_cf, " (" << (void*)lbase << ")... ");

  dwarf_fd_ = open(debug_info_path.c_str(), O_RDONLY);

  if (dwarf_fd_ == -1)
  {
    Dout(dc::finish | error_cf, "failed to open");
    return;
  }

  dwarf_handle_ = dwarf_begin(dwarf_fd_, DWARF_C_READ);
  if (!dwarf_handle_)
  {
    Dout(dc::finish, "failed to obtain DWARF handle: " << dwarf_errmsg(-1));
    close(dwarf_fd_);
    dwarf_fd_ = -1;
    return;
  }

  Dout(dc::finish, "done");
}

void ObjectFileData::close_dwarf()
{
  if (dwarf_handle_)
  {
    dwarf_end(dwarf_handle_);
    dwarf_handle_ = nullptr;
  }

  if (dwarf_fd_ >= 0)
  {
    close(dwarf_fd_);
    dwarf_fd_ = -1;
  }
}

#ifdef HAVE_DLOPEN
namespace {

// Real dynamic-loader entry points used by the exported wrappers below.  The
// wrappers are responsible for keeping dwarf's loaded-segment map synchronized
// with libraries added by dlopen and, eventually, removed by dlclose.
extern "C" {
static union
{
  void* symptr;
  void* (*func)(char const*, int);
} real_dlopen;
static union
{
  void* symptr;
  int (*func)(void*);
} real_dlclose;
}
std::once_flag initialize_real_dlopen;
std::once_flag initialize_real_dlclose;

struct DynamicLoaderRecord
{
  int refcount_;
  int flags_;
  ObjectFile const* object_file_;

  DynamicLoaderRecord(ObjectFile const* object_file, int flags) : refcount_(1), flags_(flags), object_file_(object_file)
  {
  }
};

using dynamic_loader_records_ts = threadsafe::Unlocked<std::map<void*, DynamicLoaderRecord, std::less<void*>>,
                                                       threadsafe::policy::Primitive<std::mutex>>;

dynamic_loader_records_ts& dlopen_map()
{
  static dynamic_loader_records_ts* map = new dynamic_loader_records_ts; // Intentionally leaked for late DSO teardown.
  return *map;
}

} // namespace
#endif // HAVE_DLOPEN

} // namespace dwarf
} // namespace libcwd

#ifdef HAVE_DLOPEN
extern "C" {

void* dlopen(char const* name, int flags)
{
  using namespace libcwd::dwarf;

  // No need to register if no name is given.
  bool const need_register_object_file = name && *name;

  // Ensure the initial dwarf object/segment cache exists before the new DSO is mapped.
  // That makes the following post-dlopen pass responsible only for the just-loaded link_map entry, and avoids duplicate
  // startup registration.
  if (need_register_object_file)
    libcwd::initialize();

  // Initialize real_dlopen if that wasn't done yet.
  std::call_once(initialize_real_dlopen, []() { real_dlopen.symptr = dlsym(RTLD_NEXT, "dlopen"); });

  void* handle = real_dlopen.func(name, flags);
  if (statically_linked)
  {
    Dout(dc::warning,
         "Calling dlopen(3) from statically linked application; this is not going to work if the loaded module uses "
         "libcwd too or when it allocates any memory!");
    return handle;
  }
  if (handle == nullptr)
    return handle;
#ifdef RTLD_NOLOAD
  if ((flags & RTLD_NOLOAD))
    return handle;
#endif

  // If the record already exist, increment DynamicLoaderRecord::refcount_ and return.
  {
    dynamic_loader_records_ts::wat dynamic_loader_records_w(dlopen_map());
    {
      auto iter = dynamic_loader_records_w->find(handle);
      if (iter != dynamic_loader_records_w->end())
      {
        ++iter->second.refcount_;
        iter->second.flags_ |= flags;
        return handle;
      }
    }
  }

  if (need_register_object_file)
  {
    struct link_map* link_map;
    if (::dlinfo(handle, RTLD_DI_LINKMAP, &link_map) == 0)
    {
      uintptr_t const lbase = static_cast<uintptr_t>(link_map->l_addr);
      ObjectFile const* object_file = nullptr;
      bool skip_dlopen_map = false;
      ForceLoadingDebugOutput scoped_;
      object_file = ObjectFileRegistry::register_object_file_at_lbase(lbase);
      // NULL would mean that there is no DSO at the `l_addr` that `dlinfo` just returned.
      // That shouldn't be possible because no thread can have dlclose-d it without already haven gotten the handle that
      // we didn't even return yet. However, in theory it can also mean that this DSO didn't have any non-empty PT_LOAD
      // segments, in which case we just want to ignore the DSO.
      if (!object_file)
        skip_dlopen_map = true;
      if (!skip_dlopen_map)
        dynamic_loader_records_ts::wat(dlopen_map())->emplace(handle, DynamicLoaderRecord{object_file, flags});
    }
  }

  // Unlock dlopen_map and return handle.
  return handle;
}

int dlclose(void* handle)
{
  using namespace libcwd::dwarf;

  // Initialize real_dlclose if that wasn't done yet.
  std::call_once(initialize_real_dlclose, []() { real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose"); });

  int error = real_dlclose.func(handle);
  if (error != 0 || statically_linked)
    return error;

  ObjectFile const* object_file_to_unregister = nullptr;

  // Remove the corresponding DynamicLoaderRecord from the dlopen_map if this was the last reference.
  {
    dynamic_loader_records_ts::wat dynamic_loader_records_w(dlopen_map());

    auto iter = dynamic_loader_records_w->find(handle);
    if (iter != dynamic_loader_records_w->end() && --iter->second.refcount_ == 0)
    {
#ifdef RTLD_NODELETE
      if (!(iter->second.flags_ & RTLD_NODELETE))
#endif
        object_file_to_unregister = iter->second.object_file_;
      dynamic_loader_records_w->erase(iter);
    }
  }

  if (object_file_to_unregister)
  {
    ForceLoadingDebugOutput scoped_;
    ObjectFile const* obsolete_object_file =
        object_file_to_unregister->write_locked_data()->unregister_object_file_ranges(object_file_to_unregister);
    // Now that the object_file_data_ts Access object has been destroyed, we can delete the ObjectFile that is no longer
    // in use.
    delete obsolete_object_file;
  }

  // Unlock dlopen_map and return success.
  return 0;
}

} // extern "C"
#endif // HAVE_DLOPEN

namespace libcwd {

ObjectFileName::ObjectFileName(char const* filepath) : no_debug_line_sections_(false)
{
  filepath_ = strcpy((char*)malloc(strlen(filepath) + 1), filepath); // LEAK8
  filename_ = strrchr(filepath_, '/') + 1;
  if (filename_ == (char const*)1)
    filename_ = filepath_;
}

/** @addtogroup group_locations */
/** \{ */

char const* const unknown_function_c = "<unknown function>";

/**
 * @brief Find the mangled function name of the address @p addr.
 *
 * @returns the same pointer that is returned by Location::mangled_function_name() on success,
 * otherwise @ref unknown_function_c is returned.
 *
 * Note: the returned pointer is invalidated by calling dlclose(3) on the DSO that contains the returned function!
 */
char const* pc_mangled_function_name(void const* pc)
{
  using namespace dwarf;

  LIBCWD_TSD_DECLARATION;

  // The call to find_object_file also load the symbols for the object file (if not already done before).
  ObjectFile const* object_file = static_cast<ObjectFile const*>(find_object_file(pc, LIBCWD_TSD));
  if (!object_file)
    return unknown_function_c;

  SymbolRangeInterface const* symbol = object_file->find_symbol(reinterpret_cast<uintptr_t>(pc));
  if (!symbol)
    return unknown_function_c;

  return symbol->name();
}

/** \} */ // End of group 'group_locations'.

} // namespace libcwd

#endif // CWDEBUG_LOCATION
