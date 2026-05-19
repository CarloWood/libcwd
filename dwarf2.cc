#include "cwd_sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "cwd_dwarf2.h"
#include <libcwd/ObjectFileName.h>
#include "libcwd/debug.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <dlfcn.h>
#include <elf.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>
#include <filesystem>
#include <fcntl.h>
#include <link.h>
#include <limits>
#include <map>
#include <string>
#ifdef HAVE_DLOPEN
#include <mutex>
#endif

namespace libcwd {

// New debug channels.
namespace channels {
namespace dc {
/** \addtogroup group_default_dc */
/** \{ */

/** The BFD channel. */
channel_ct bfd
#ifndef HIDE_FROM_DOXYGEN
  ("BFD")
#endif
  ;

// Internal debug channel.
channel_ct dwarf
#ifndef HIDE_FROM_DOXYGEN
  ("DWARF")
#endif
  ;

/** \} */
} // namespace dc
} // namespace channels

namespace dwarf2 {

// Detect if this libcwd library is static or not.
#ifdef __PIC__
constexpr bool statically_linked = false;
#else
constexpr bool statically_linked = true;
#endif
static bool WST_initialized = false;                      // MT: Set here to false, set to `true' once in `ST_init'.

//static
ObjectFileBase::object_files_t ObjectFileBase::s_object_files_;

// class ObjectFileRegistry
//
// Owns the static object-file registry operations and initialization state for
// ObjectFile.  The underlying segment map itself lives in ObjectFileBase;
// ObjectFileRegistry adds the dl_iterate_phdr based population helpers that
// create ObjectFile instances and publish their PT_LOAD ranges in that map.
//
// To get access to the list you need to read and/or write lock it.
//
// To obtain a read lock, create a stack variable,
//
//   object_files_t::rat object_files_r(s_object_files_);       // Scoped read-lock for s_object_files_.
//
// Use object_files_r-> for read access (returns a std::map<uintptr_t, PTLoadSegment> const*).
//
// To obtain a write lock, create a stack variable,
//
//   object_files_t::wat object_files_w(s_object_files_);       // Scoped write-lock for s_object_files_.
//
// Use object_files_w-> for write access (returns a std::map<uintptr_t, PTLoadSegment>*).
//
class ObjectFileRegistry : public ObjectFileBase
{
  static std::atomic_bool s_object_files_initialized_;

 protected:
  // Construct the ObjectFileBase subobject for the derived ObjectFile payload.
  // ObjectFileRegistry is not instantiated by itself; it only groups the
  // registry-wide static functions and state while preserving the original base
  // class storage for object_file_name_.
  ObjectFileRegistry(char const* filename) : ObjectFileBase(filename) { }

 public:
  // Information passed to the cb_dl_iterate_phdr call back function.
  struct CallBackData {
    std::string executable_path_;               // The full path to the current executable.
    // Used for targeted dlopen-driven object discovery:
    uintptr_t target_lbase_;                    // When non-zero, create an ObjectFile only for the DSO with this load base.
    mutable object_file_t* object_file_{};      // Set to the newly created ObjectFile wrapper for target_lbase_.

    CallBackData(std::string const& executable_path, uintptr_t target_lbase = 0) :
      executable_path_(executable_path), target_lbase_(target_lbase) { }
  };

  friend bool dwarf2::ST_init(LIBCWD_TSD_PARAM);
  static void register_initial_object_files();
  static object_file_t* iterate_program_headers(CallBackData const& data);
  static int cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data);
  static object_file_t* find_registered_object_file(uintptr_t lbase, object_files_t::wat const& object_files_w);

  // Called from dlopen.
  static object_file_t* register_object_file_at_lbase(uintptr_t lbase);
};

// class ObjectFileData
//
// Represents a single object file: either the executable or a shared library.
// Registry-wide operations live in ObjectFileRegistry; instances only own the
// per-object load base and libdw state.
class ObjectFileData : public ObjectFileRegistry
{
 private:
  uintptr_t const lbase_;
  mutable Dwarf* dwarf_handle_{nullptr};        // mutable because close_dwarf() sets these to nullptr and -1 again.
  mutable int dwarf_fd_{-1};

 public:
  // Accessors.
  uintptr_t lbase() const { return lbase_; }
  bool is_initialized() const { return dwarf_fd_ != -1; }

  // Construct and destroy the protected ObjectFile payload inside object_file_t.
  // Construction records the path/load base and currently still performs eager
  // symbol loading; later objectives will move that mutable work behind the
  // wrapper lock.  Destruction releases any libdw resources owned by the payload.
  ObjectFileData(char const* filename, uintptr_t lbase);
  ~ObjectFileData();

 private:
  // Called by constructor.
  void load_symbols();

  void open_dwarf();
  void close_dwarf() const;

 public:
  // Called from dlclose.
  void unregister_object_file_ranges(object_file_t const* self) const;
};

ObjectFileData::ObjectFileData(char const* filename, uintptr_t lbase) : ObjectFileRegistry(filename), lbase_(lbase)
{
  Dout(dc::dwarf, "new ObjectFile \"" << filename << "\" with load base 0x" << std::hex << lbase);
  load_symbols();
}

ObjectFileData::~ObjectFileData()
{
  Dout(dc::dwarf, "destroying ObjectFile \"" << object_file_name_.filepath() << "\".");
  close_dwarf();
}

//static
std::atomic_bool ObjectFileRegistry::s_object_files_initialized_ = false;

namespace {

#ifdef HAVE_DLOPEN

struct Chain;

// This is non-zero while inside `dwfl_module_getdwarf`.
thread_local Chain* inside_dwfl_module_getdwarf = nullptr;

// struct Chain
//
// A chain of not-yet-registered DSO handle plus load address.
//
//                                 .-------Chain-------.
// inside_dwfl_module_getdwarf --> | On the stack      |
//         ^                       | {handle_, lbase_} |   .-------Chain-------.
//         |                       |    ^        next_ +-->| Heap allocated    |
//         |                       '----|--------------'   | {handle_, lbase_} |             .-------Chain-------.
//      or nullptr if not               |                  |             next_ +--> [...] -->| Heap allocated    |
//  inside dwfl_module_getdwarf      or nullptr if         '-------------------'             | {handle_, lbase_} |
//                                 the list is empty.                                        |             next_ +--> nullptr
//                                                                                           '-------------------'
//
// The whole chain is single threaded: each thread has it's own list.
//
struct Chain
{
  void* handle_ = nullptr;              // Set if this Chain instance represents a yet-to-be registered DSO.
  Chain* next_ = nullptr;               // Set if there is more than one such DSO.
  uintptr_t lbase_;                     // The load address of the DSO corresponding to handle_. Only valid if handle_ is non-zero.

  // Append one DSO to the end of the list.
  static void append(void* handle, uintptr_t lbase);

  // Remove one DSO from the list.
  static bool remove(void* handle);

  // Perform the delayed initialization and free any allocated nodes.
  void delayed_initialization();
};

// Append one DSO that was opened while this thread was inside dwfl_module_getdwarf().
// Actual ObjectFile construction is deferred until the outermost dwfl_module_getdwarf()
// call returns because elfutils can dlopen libdebuginfod from inside a pthread_once initializer;
// recursively entering dwfl_module_getdwarf from our dlopen wrapper would self-deadlock there.
//
//<static>
void Chain::append(void* handle, uintptr_t lbase)
{
  Chain* node = inside_dwfl_module_getdwarf;

  // In the fast path we only get here once, and then handle_ is still null;
  // so normally we can avoid allocation and only allocate when that is necessary (the second call, that frankly should never happen).
  if (node->handle_)  // List not empty?
  {
    // Append a new Chain to the end of the list and have `node` point to that.
    while (node->next_)
      node = node->next_;
    node->next_ = new Chain;
    node = node->next_;
  }
  node->handle_ = handle;
  node->lbase_ = lbase;
}

//static
bool Chain::remove(void* handle)
{
  Chain* prev = nullptr;
  Chain* node = inside_dwfl_module_getdwarf;

  if (node->handle_)  // List not empty?
  {
    do
    {
      if (node->handle_ == handle)
      {
        if (!prev)      // Is this the first node?
        {
          // We can't remove the first node: that is the one on the stack.
          // Instead we replace its contents.
          if (!node->next_)             // Is this the only element?
            node->handle_ = nullptr;
          else
          {
            Chain* second_node = node->next_;
            *node = *second_node;
            delete second_node;
          }
        }
        else
        {
          // Remove `node` from the list.
          prev->next_ = node->next_;
          delete node;
        }
        return true;
      }
      prev = node;
      node = node->next_;
    }
    while (node);
  }
  return false;
}

#endif // HAVE_DLOPEN

// Return the ELF file that should be opened for DWARF data for object_file.
// Downloads of missing debug info by debuginfod can happen if DEBUGINFOD_URLS is set,
// otherwise libdwfl can still use already installed local debuginfo and ordinary
// debuglink paths.
//
// The returned path is either the separate debuginfo file selected by libdwfl or
// object_file itself when no separate file was found; the caller should open the
// path with libdw which reports missing/no-DWARF cases.
//
std::filesystem::path get_debug_info_path(std::string const& object_file)
{
  Dwfl_Callbacks callbacks{};
  callbacks.find_debuginfo = dwfl_standard_find_debuginfo;
  callbacks.section_address = dwfl_offline_section_address;

  Dwfl* dwfl = dwfl_begin(&callbacks);
  if (!dwfl)
  {
    Dout(dc::warning, "dwfl_begin failed while looking for debuginfo of \"" << object_file << "\": " << dwfl_errmsg(-1));
    return object_file;
  }

  struct DwflCloser {
    Dwfl* dwfl_;
    ~DwflCloser() { dwfl_end(dwfl_); }
  } close_dwfl{dwfl};

  Dwfl_Module* module = dwfl_report_offline(dwfl, object_file.c_str(), object_file.c_str(), -1);
  if (!module)
  {
    Dout(dc::warning, "dwfl_report_offline failed for \"" << object_file << "\": " << dwfl_errmsg(-1));
    return object_file;
  }

  if (dwfl_report_end(dwfl, nullptr, nullptr) != 0)
  {
    Dout(dc::warning, "dwfl_report_end failed for \"" << object_file << "\": " << dwfl_errmsg(-1));
    return object_file;
  }

#ifdef HAVE_DLOPEN
  // The call to dwfl_module_getdwarf can cause a call to dlopen("libdebuginfod.so.1").
  // If that happens we shouldn't try to register the opened DSO because that means we might
  // end up here again, and dwfl_module_getdwarf itself is not re-entrant (it will deadlock).
  Chain chain;
  inside_dwfl_module_getdwarf = &chain;         // Mark that we are inside dwfl_module_getdwarf.
#endif // HAVE_DLOPEN

  // Force libdwfl to locate and validate the separate debuginfo file now; only
  // after this call does dwfl_module_info reliably expose the selected debugfile.
  Dwarf_Addr bias;
  (void)dwfl_module_getdwarf(module, &bias);

#ifdef HAVE_DLOPEN
  inside_dwfl_module_getdwarf = nullptr;        // We returned from dwfl_module_getdwarf.

  if (chain.handle_)                            // Was dlopen called, at least once, from within dwfl_module_getdwarf?
    chain.delayed_initialization();
#endif // HAVE_DLOPEN

  char const* mainfile = nullptr;
  char const* debugfile = nullptr;
  dwfl_module_info(module, nullptr, nullptr, nullptr, nullptr, nullptr, &mainfile, &debugfile);

  if (debugfile && debugfile[0] != '\0')
    return debugfile;
  if (mainfile && mainfile[0] != '\0')
    return mainfile;
  return object_file;
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
// The result is used only for diagnostics while building the dwarf2 object/segment cache.
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

  libcwd::debug_ct::OnOffState do_state;
  libcwd::channel_ct::OnOffState bfd_state;
  libcwd::channel_ct::OnOffState dwarf_state;

  ForceLoadingDebugOutput()
  {
    if (forced_loading_output)
    {
      Debug(libcw_do.force_on(do_state));
      Debug(dc::bfd.force_on(bfd_state, "BFD"));
      Debug(dc::dwarf.force_on(dwarf_state, "DWARF"));
    }
  }

  ~ForceLoadingDebugOutput()
  {
    if (forced_loading_output)
    {
      Debug(dc::dwarf.restore(dwarf_state));
      Debug(dc::bfd.restore(bfd_state));
      Debug(libcw_do.restore(do_state));
    }
  }
};

} // namespace

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

    // This is the initial population pass for the dwarf2 object/segment cache.
    // Even if dlopen is called first, that still will first call this function.
    LIBCWD_ASSERT(object_files_t::rat{s_object_files_}->empty());

    // Load executable and shared objects.
    if (!statically_linked)
    {
      // Iterate over all currently loaded object files and create new ObjectFile's for them.
      iterate_program_headers(data);
    }

    s_object_files_initialized_ = true;
  } // Unlock s_object_files_.
}

//static
object_file_t* ObjectFileRegistry::iterate_program_headers(CallBackData const& data)
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
int ObjectFileRegistry::cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data)
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
  char const* object_filename = (info->dlpi_name && info->dlpi_name[0] != '\0') ? info->dlpi_name : data->executable_path_.c_str();

  // A DSO can be discovered through a delayed dlopen registration while an
  // outer dl_iterate_phdr pass is still running.  Avoid constructing and
  // inserting a duplicate ObjectFile when the iterator later reaches the same
  // link-map entry.
  {
    object_files_t::wat object_files_w(s_object_files_);
    if (object_file_t* existing_object_file = find_registered_object_file(lbase, object_files_w))
    {
      data->object_file_ = existing_object_file;
      return target_lbase_only;
    }
  }

  // Create a new ObjectFile wrapper.  The wrapper address, not the protected
  // ObjectFile payload address, is stored in every PTLoadSegment so all future
  // users are forced through threadsafe access objects before touching mutable
  // per-object DWARF state.
  data->object_file_ = new object_file_t(object_filename, lbase);

  // Insert one immutable PTLoadSegment per loadable segment into the end-keyed map.
  // The map owns the discoverable active address ranges; the pointed-to objects are intentionally kept stable
  // for the process lifetime until later dlclose/tombstone semantics are designed.
  for (ElfW(Half) phdr_index = 0; phdr_index < info->dlpi_phnum; ++phdr_index)
  {
    ElfW(Phdr) const& phdr = info->dlpi_phdr[phdr_index];
    if (phdr.p_type != PT_LOAD ||
        // The Elf-64 Object File Format specifies that p_memsz may be zero (https://man7.org/linux/man-pages/man5/elf.5.html)
        phdr.p_memsz == 0)
      continue;

    uintptr_t const segment_start = static_cast<uintptr_t>(info->dlpi_addr + phdr.p_vaddr);
    uintptr_t const segment_end = segment_start + static_cast<uintptr_t>(phdr.p_memsz);
    LIBCWD_ASSERT(segment_end > segment_start);

    object_files_t::wat object_files_w(s_object_files_);
    auto const ibp = object_files_w->try_emplace(segment_end, data->object_file_, lbase, segment_start, segment_end, phdr.p_flags);
    // Make sure the new segment doesn't overlap with an already existing one.
    LIBCWD_ASSERT(ibp.second &&
        (ibp.first == object_files_w->begin() ||
            std::prev(ibp.first)->first <= segment_start) &&
        (std::next(ibp.first) == object_files_w->end() ||
            std::next(ibp.first)->second.start_addr() >= segment_end));

    Dout(dc::dwarf, "    map[" << (void*)segment_end << "] = PTLoadSegment " << ibp.first->second <<
        " [" << (void*)segment_start << '-' << (void*)segment_end << ") flags=" << flags_to_string(phdr.p_flags) <<
        " object=" << data->object_file_);
  }

  // Stop iterating iff there was only one target.
  return target_lbase_only;
}

//static
object_file_t* ObjectFileRegistry::find_registered_object_file(uintptr_t lbase, object_files_t::wat const& object_files_w)
{
  // The writable map is keyed by PT_LOAD end address and can contain multiple
  // segments for the same object file.  Treat any segment whose owning
  // ObjectFile has the requested load base as proof that the object was already
  // registered; this prevents duplicate ObjectFile creation when loader
  // enumeration reports a DSO that was discovered through an earlier path.
  for (auto const& [segment_end, segment] : *object_files_w)
  {
    if (segment.object_lbase() == lbase)
      return segment.object_file();
  }
  return nullptr;
}

//static
object_file_t* ObjectFileRegistry::register_object_file_at_lbase(uintptr_t lbase)
{
  // Locks s_object_files_.
  CallBackData const data{current_executable_path(), lbase};

  // If this object file was already loaded before, then return that instead of creating a new ObjectFile.
  object_file_t* existing_object_file = find_registered_object_file(lbase, object_files_t::wat{s_object_files_});

  // Otherwise iterate over all currently loaded object files to find the just dynamically
  // opened object file, loaded at data.target_lbase_, and create a new ObjectFile for it.
  return existing_object_file ? existing_object_file : iterate_program_headers(data);
}

void ObjectFileData::unregister_object_file_ranges(object_file_t const* self) const
{
  // Remove all PTLoadSegment's from s_object_files_ that belong to this ObjectFile.
  // `self` is the stable wrapper pointer stored in each PTLoadSegment; compare
  // that pointer directly so this write-locked ObjectFile does not need to take
  // a nested read lock on itself while erasing its ranges.
  {
    object_files_t::wat object_files_w(s_object_files_);
    for (auto iter = object_files_w->begin(); iter != object_files_w->end(); )
    {
      PTLoadSegment const& segment = iter->second;
      if (segment.object_file() == self)
      {
        Dout(dc::dwarf, "removing map[" << (void*)iter->first << "] = PTLoadSegment " << segment <<
            " for object " << this << " (\"" << object_file_name_.filename() << "\")");
        iter = object_files_w->erase(iter);
      }
      else
        ++iter;
    }
  }
  // Free libdw resources.
  close_dwarf();
}

void ObjectFileData::load_symbols()
{
  open_dwarf();
}

bool ST_init(LIBCWD_TSD_PARAM)
{
  static bool WST_being_initialized = false;
  // Detect recursive initialization.
  if (WST_being_initialized)
    return false;
  WST_being_initialized = true;

  // This must be called before calling ST_init().
  if (!libcw_do.NS_init(LIBCWD_TSD))
    return false;

  // MT: We assume this is called before reaching main().
  //     Therefore, no synchronisation is required.
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
  if (_private_::WST_multi_threaded)
    core_dump();
#endif

  ObjectFileRegistry::register_initial_object_files();

  return true;
}

void ObjectFileData::open_dwarf()
{
  // Should only be called once (from load_symbols() which is called from the constructor).
  LIBCWD_ASSERT(dwarf_fd_ == -1 && dwarf_handle_ == nullptr);

  std::string debug_info_path = get_debug_info_path(object_file_name_.filepath());

  bool different_symbols_path = debug_info_path != object_file_name_.filepath();
  Dout(dc::bfd|continued_cf, "Loading debug info " << (different_symbols_path ? "for " : "from ") << object_file_name_.filepath());
  if (different_symbols_path)
    Dout(dc::continued, " (from " << debug_info_path << ")");
  Dout(dc::continued|flush_cf, " (" << (void*)lbase_ << ")... ");

  dwarf_fd_ = open(debug_info_path.c_str(), O_RDONLY);

  if (dwarf_fd_ == -1)
  {
    Dout(dc::finish|error_cf, "failed to open");
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

void ObjectFileData::close_dwarf() const
{
  if (dwarf_handle_)
  {
    dwarf_end(dwarf_handle_);
    dwarf_handle_ = nullptr;
  }

  if (dwarf_fd_ != -1)
  {
    close(dwarf_fd_);
    dwarf_fd_ = -1;
  }
}

#ifdef HAVE_DLOPEN
namespace {

// Real dynamic-loader entry points used by the exported wrappers below.  The
// wrappers are responsible for keeping dwarf2's loaded-segment map synchronized
// with libraries added by dlopen and, eventually, removed by dlclose.
extern "C" {
static union { void* symptr; void* (*func)(char const*, int); } real_dlopen;
static union { void* symptr; int (*func)(void*); } real_dlclose;
}
std::once_flag initialize_real_dlopen;
std::once_flag initialize_real_dlclose;

struct DynamicLoaderRecord
{
  int refcount_;
  int flags_;
  object_file_t* object_file_;

  DynamicLoaderRecord(object_file_t* object_file, int flags) : refcount_(1), flags_(flags), object_file_(object_file) { }
};

using dynamic_loader_records_t = threadsafe::Unlocked<std::map<void*, DynamicLoaderRecord, std::less<void*>>, threadsafe::policy::Primitive<std::mutex>>;

dynamic_loader_records_t& dlopen_map()
{
  static dynamic_loader_records_t* map = new dynamic_loader_records_t;  // Intentionally leaked for late DSO teardown.
  return *map;
}

void Chain::delayed_initialization()
{
  for (Chain* node = this; node; node = node->next_)
  {
    ForceLoadingDebugOutput scoped_;
    object_file_t* object_file = ObjectFileRegistry::register_object_file_at_lbase(node->lbase_);
    LIBCWD_ASSERT(object_file);
  }
}

} // namespace
#endif // HAVE_DLOPEN

} // namespace dwarf2
} // namespace libcwd

#ifdef HAVE_DLOPEN
extern "C" {

void* dlopen(char const* name, int flags)
{
  using namespace libcwd::dwarf2;

  // No need to register if no name is given.
  bool const need_register_object_file = name && *name;

  // Ensure the initial dwarf2 object/segment cache exists before the new DSO is mapped.
  // That makes the following post-dlopen pass responsible only for the just-loaded link_map entry, and avoids duplicate startup registration.
  if (need_register_object_file)
    libcwd::initialize();

  // Initialize real_dlopen if that wasn't done yet.
  std::call_once(initialize_real_dlopen, [](){ real_dlopen.symptr = dlsym(RTLD_NEXT, "dlopen"); });

  void* handle = real_dlopen.func(name, flags);
  if (statically_linked)
  {
    Dout(dc::warning, "Calling dlopen(3) from statically linked application; this is not going to work if the loaded module uses libcwd too or when it allocates any memory!");
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
    dynamic_loader_records_t::wat dynamic_loader_records_w(dlopen_map());
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
      object_file_t* object_file = nullptr;
      // If this thread is inside dwfl_module_getdwarf then do not register the DSO.
      if (inside_dwfl_module_getdwarf)
        Chain::append(handle, lbase);
      else
      {
        ForceLoadingDebugOutput scoped_;
        object_file = ObjectFileRegistry::register_object_file_at_lbase(lbase);
        // NULL would mean that there is no DSO at the `l_addr` that `dlinfo` just returned.
        // That shouldn't be possible because no thread can have dlclose-d it without already haven gotten the handle that we didn't even return yet.
        LIBCWD_ASSERT(object_file);
      }
      dynamic_loader_records_t::wat(dlopen_map())->emplace(handle, DynamicLoaderRecord{object_file, flags});
    }
  }

  // Unlock dlopen_map and return handle.
  return handle;
}

int dlclose(void* handle)
{
  using namespace libcwd::dwarf2;

  // Initialize real_dlclose if that wasn't done yet.
  std::call_once(initialize_real_dlclose, [](){ real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose"); });

  int error = real_dlclose.func(handle);
  if (error != 0 || statically_linked)
    return error;

  object_file_t* object_file_to_unregister = nullptr;

  // Remove the corresponding DynamicLoaderRecord from the dlopen_map if this was the last reference.
  {
    dynamic_loader_records_t::wat dynamic_loader_records_w(dlopen_map());

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

  if (object_file_to_unregister &&
      !(inside_dwfl_module_getdwarf && Chain::remove(handle)))
  {
    ForceLoadingDebugOutput scoped_;
    object_file_t::wat object_file_w(*object_file_to_unregister);
    object_file_w->unregister_object_file_ranges(object_file_to_unregister);
  }

  // Unlock dlopen_map and return success.
  return 0;
}

} // extern "C"
#endif // HAVE_DLOPEN

namespace libcwd {

ObjectFileName::ObjectFileName(char const* filepath) : no_debug_line_sections_(false)
{
  LIBCWD_TSD_DECLARATION;
  filepath_ = strcpy((char*)malloc(strlen(filepath) + 1), filepath);	// LEAK8
  filename_ = strrchr(filepath_, '/') + 1;
  if (filename_ == (char const*)1)
    filename_ = filepath_;
}

} // namespace libcwd

#endif // CWDEBUG_LOCATION
