// SPDX-FileCopyrightText: 2000-2007, 2009, 2013, 2017-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include <libcwd/config.h>
#include "macros.h"
#include "private_DebugStack.inl.h"
#include "threadsafe/AIReadWriteMutex.h"
#include "threadsafe/threadsafe.h"
#if CWDEBUG_LOCATION
#include "cwd_dwarf.h"
#endif
#include "libcwd/debug.h"
#include <libcwd/Location.inl.h>
#include <libcwd/private/BufferStream.h>
#include <libcwd/strerrno.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib> // Needed for _Exit() (C99)
#include <iostream>
#include <new>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h> // Needed for setrlimit()
#include <sys/time.h> // Needed for setrlimit()
#endif
#include <sstream>
#include <vector>
#include "cwd_debug.h"

extern "C" int raise(int);

namespace libcwd {

class Buffer : public std::stringbuf
{
 private:
  using streampos_t = pos_type;
  streampos_t position_;
  // These two are protected by the ostream lock of a debug object.
  bool unfinished_already_printed_;
  bool continued_needed_;

 public:
  Buffer() : unfinished_already_printed_(false), continued_needed_(false) { }
  void writeto(std::ostream* os, LIBCWD_TSD_PARAM, DebugObject& debug_object, bool request_unfinished,
               bool do_flush, bool ends_on_newline, bool possible_nonewline_cf);
  void store_position() { position_ = this->pubseekoff(0, std::ios_base::cur, std::ios_base::out); }
  void restore_position()
  {
    this->pubseekpos(position_, std::ios_base::out);
    this->pubseekpos(0, std::ios_base::in);
    continued_needed_ = false;
  }
  void continued() { unfinished_already_printed_ = false; }
  void write_prefix_to(std::ostream* os)
  {
    streampos_t old_in_pos = this->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
    this->pubseekpos(0, std::ios_base::in);
    os->put(this->sgetc());
    int size = position_ - std::streampos(0);
    for (int c = 1; c < size; ++c)
      os->put(this->snextc());
    this->pubseekpos(old_in_pos, std::ios_base::in);
  }
};

namespace _private_ {
extern std::atomic_bool WST_multi_threaded;
} // namespace _private_

void Buffer::writeto(std::ostream* os, LIBCWD_TSD_PARAM, DebugObject& debug_object, bool request_unfinished,
                     bool do_flush, bool ends_on_newline, bool possible_nonewline_cf)
{
  // os			: The ostream that we need to write to.
  // __libcwd_tsd		: The Thread Specific context.
  // debug_object		: The debug object context.
  // request_unfinished	: When set, then this thread is writing output that is interrupting
  //			  unfinished debug output of its own.
  // do_flush		: Flush the ostream after writing to it.
  // ends_on_newline	: This output ends on a newline.
  // possible_nonewline_cf	: When `ends_on_newline' is false, then that was caused by the use of nonewline_cf.

  using msgbuf_t = char*;
  msgbuf_t msgbuf;
  bool const queue_msg = false;
  int const extra_size = 0;
  int curlen;
  curlen = this->pubseekoff(0, std::ios_base::cur, std::ios_base::out) -
           this->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  bool free_msgbuf = false;
  if (queue_msg)
    msgbuf = (msgbuf_t)malloc(curlen + extra_size);
  else if (curlen > 512 || !(msgbuf = (msgbuf_t)__builtin_alloca(curlen + extra_size)))
  {
    msgbuf = (msgbuf_t)malloc(curlen + extra_size);
    free_msgbuf = true;
  }
  this->sgetn(msgbuf, curlen);
  std::ostream* locked_os;
  _private_::LockInterfaceBase* locked_mutex;
  locked_os = debug_object.ostream_state_.get_locked_os(os, &locked_mutex);
  if (locked_mutex)
    __libcwd_tsd.lock_interface_is_locked = true;
  if (!locked_mutex && _private_::WST_multi_threaded.load(std::memory_order_relaxed))
  {
    static bool WST_second_time = false; // Break infinite loop.
    if (!WST_second_time)
    {
      WST_second_time = true;
      DoutFatal(dc::core,
                "When using multiple threads, you must provide a locking mechanism for the debug output stream.  "
                "You can pass a pointer to a mutex with `DebugObject::set_ostream' (see "
                "documentation/group__group__destination.html).");
    }
  }
  if (debug_object.newlineless_tsd_ && debug_object.newlineless_tsd_ != &__libcwd_tsd)
  {
    _private_::ThreadSpecificData const* newlineless_tsd =
        static_cast<_private_::ThreadSpecificData const*>(debug_object.newlineless_tsd_);
    DebugString const& color_off = newlineless_tsd->debug_object_array[debug_object.index_]->color_off;
    size_t color_off_size = color_off.size();
    if (color_off_size > 0)
      locked_os->write(color_off.c_str(), color_off_size);
    if (debug_object.unfinished_oss_)
    {
      if (debug_object.unfinished_oss_ != this)
      {
        locked_os->write("<unfinished>\n", 13);
        debug_object.unfinished_oss_->unfinished_already_printed_ = true;
        debug_object.unfinished_oss_->continued_needed_ = true;
      }
    }
    else
      locked_os->write("<no newline>\n", 13);
  }
  DebugString const& color_off = LIBCWD_DO_TSD_MEMBER(debug_object, color_off);
  size_t color_off_size = color_off.size();
  if (continued_needed_ && curlen > 0)
  {
    continued_needed_ = false;
    write_prefix_to(locked_os);
    if (color_off_size > 0)
      locked_os->write(color_off.c_str(), color_off_size);
    DebugString const& color_on = LIBCWD_DO_TSD_MEMBER(debug_object, color_on);
    size_t color_on_size = color_on.size();
    if (color_on_size > 0)
    {
      locked_os->write("<continued>", 11);
      locked_os->write(color_on.c_str(), color_on_size);
      locked_os->put(' ');
    }
    else
      locked_os->write("<continued> ", 12);
    continued();
  }
  locked_os->write(msgbuf, curlen);
  if (request_unfinished && !unfinished_already_printed_)
  {
    if (color_off_size > 0)
      locked_os->write(color_off.c_str(), color_off_size);
    locked_os->write("<unfinished>\n", 13);
  }
  if (do_flush)
    locked_os->flush();
  unfinished_already_printed_ = ends_on_newline;
  if (ends_on_newline)
  {
    debug_object.unfinished_oss_ = NULL;
    debug_object.newlineless_tsd_ = NULL;
  }
  else if (curlen > 0)
  {
    debug_object.newlineless_tsd_ = &__libcwd_tsd;
    if (possible_nonewline_cf)
      debug_object.unfinished_oss_ = NULL;
    else
      debug_object.unfinished_oss_ = this;
  }
  if (locked_mutex)
  {
    __libcwd_tsd.lock_interface_is_locked = false;
    locked_mutex->unlock();
  }
  if (free_msgbuf)
    free(msgbuf);
}

// Configuration signature
unsigned long const config_signature_lib_c = config_signature_header_c;

// Return the configuration signature of the .so file.
unsigned long get_config_signature_lib_c()
{
  return config_signature_lib_c;
}

// Put this here to decrease the code size of `main_reached'
void conf_check_failed()
{
  DoutFatal(dc::fatal,
            "configuration check: This version of libcwd was compiled with a different configuration than is currently "
            "used in libcwd/config.h!");
}

void version_check_failed()
{
  DoutFatal(dc::fatal,
            "configuration check: This version of libcwd does not match the version of libcwd/config.h! Are your paths "
            "correct? Did you recently upgrade libcwd and forgot to recompile this application?");
}

/**
 * \brief The default %debug object.
 *
 * The %debug object that is used by default by \ref Dout and \ref DoutFatal, the only %debug object used by libcwd
 * itself.
 * \sa \ref chapter_custom_do
 */

DebugObject libcw_do;

namespace {
std::atomic<unsigned short int> WST_max_len = 8; // The length of the longest label.  Is adjusted automatically
                                                 // if a custom channel has a longer label.
} // namespace

namespace channels {
namespace dc {

/**
 * \addtogroup group_default_dc Predefined Debug Channels
 * \ingroup group_debug_channels
 *
 * These are the default %debug %channels pre-defined in libcwd.
 */

/** \{ */

/** The DEBUG channel. */
Channel debug
#ifndef HIDE_FROM_DOXYGEN
    ("DEBUG")
#endif
        ;

/** The NOTICE channel. */
Channel notice
#ifndef HIDE_FROM_DOXYGEN
    ("NOTICE")
#endif
        ;

/** The SYSTEM channel. */
Channel system
#ifndef HIDE_FROM_DOXYGEN
    ("SYSTEM")
#endif
        ;

/** The WARNING channel.
 *
 * This is the only channel that
 * is turned on by default.
 */
Channel warning
#ifndef HIDE_FROM_DOXYGEN
    ("WARNING")
#endif
        ;

/** A special channel that is always turned on.
 *
 * This channel is <EM>%always</EM> on;
 * it can not be turned off.&nbsp;
 * It is not in the list of \ref ForAllDebugChannels "debug channels".&nbsp;
 * When used with a label it will print as many '>'
 * characters as the size of the largest real channel.
 */
AlwaysChannel always;

/** A special channel to continue to
 * write a previous %debug channel.
 *
 * \sa \ref using_continued
 */
ContinuedChannel continued
#ifndef HIDE_FROM_DOXYGEN
    (continued_maskbit)
#endif
        ;

/** A special channel to finish writing
 * <EM>%continued</EM> %debug output.
 *
 * \sa \ref using_continued
 */
ContinuedChannel finish
#ifndef HIDE_FROM_DOXYGEN
    (finish_maskbit)
#endif
        ;

/** The special FATAL channel.
 *
 * \sa DoutFatal
 */
FatalChannel fatal
#ifndef HIDE_FROM_DOXYGEN
    ("FATAL", fatal_maskbit)
#endif
        ;

/** The special COREDUMP channel.
 *
 * \sa DoutFatal
 */
FatalChannel core
#ifndef HIDE_FROM_DOXYGEN
    ("COREDUMP", coredump_maskbit)
#endif
        ;

/** \} */
} // namespace dc
} // namespace channels

/** A special channel that is always off. */
Channel const Channel::off_channel
#ifndef HIDE_FROM_DOXYGEN
    ("!NEVER!", false)
#endif
        ;

void ST_initialize_globals(LIBCWD_TSD_PARAM)
{
  static bool ST_already_called;
  if (ST_already_called)
    return;
  ST_already_called = true;
  _private_::process_environment_variables();

  // Fatal channels need to be marked fatal, otherwise we get into an endless loop
  // when they are used before they are created.
  channels::dc::core.NS_initialize("COREDUMP", coredump_maskbit, LIBCWD_TSD);
  channels::dc::fatal.NS_initialize("FATAL", fatal_maskbit, LIBCWD_TSD);
  // Initialize other debug channels that might be used before we reach main().
  channels::dc::debug.NS_initialize("DEBUG", LIBCWD_TSD, true);
  channels::dc::continued.NS_initialize(continued_maskbit);
  channels::dc::finish.NS_initialize(finish_maskbit);
#if CWDEBUG_LOCATION
  channels::dc::elfutils.NS_initialize("ELFUTILS", LIBCWD_TSD, true);
#endif
  // What the heck, initialize all other debug channels too
  channels::dc::warning.NS_initialize("WARNING", LIBCWD_TSD, true);
  channels::dc::notice.NS_initialize("NOTICE", LIBCWD_TSD, true);
  channels::dc::system.NS_initialize("SYSTEM", LIBCWD_TSD, true);

  if (!libcw_do.NS_init(LIBCWD_TSD)) // Initialize debug code.
    DoutFatal(dc::core, "Calling DebugObject::NS_init recursively from ST_initialize_globals");

  // Unlimit core size.
#ifdef RLIMIT_CORE
  struct rlimit corelim;
  if (getrlimit(RLIMIT_CORE, &corelim))
    DoutFatal(dc::fatal | error_cf, "getrlimit(RLIMIT_CORE, &corelim)");
  corelim.rlim_cur = corelim.rlim_max;
  if (corelim.rlim_max != RLIM_INFINITY && !_private_::suppress_startup_msgs)
  {
    DebugObject::OnOffState state;
    libcw_do.force_on(state);
    // The cast is necessary on platforms where corelim.rlim_max is long long
    // and libstdc++ was not compiled with support for long long.
    if (corelim.rlim_max != 0)
      Dout(dc::warning, "core size is limited (hard limit: " << (unsigned long)(corelim.rlim_max / 1024)
                                                             << " kb).  Core dumps might be truncated!");
    libcw_do.restore(state);
  }
  if (setrlimit(RLIMIT_CORE, &corelim))
    DoutFatal(dc::fatal | error_cf, "unlimit core size failed");
#else
  if (!_private_::suppress_startup_msgs)
  {
    DebugObject::OnOffState state;
    libcw_do.force_on(state);
    Dout(dc::warning, "Please unlimit core size manually");
    libcw_do.restore(state);
  }
#endif

#if CWDEBUG_LOCATION
  dwarf::ensure_initialization(LIBCWD_TSD); // Initialize DWARF code.
#endif
#if CWDEBUG_DEBUG && !CWDEBUG_DEBUGOUTPUT
  // Force allocation of a __cxa_eh_globals struct in libsupc++.
  [[maybe_unused]] int count = std::uncaught_exceptions(); // Leaks memory.
#endif
}

void initialize()
{
  LIBCWD_TSD_DECLARATION;
  ST_initialize_globals(LIBCWD_TSD);
}

#ifndef HIDE_FROM_DOXYGEN
namespace _private_ {

static std::atomic<bool> fatal_termination_started{false};

// Claim ownership of process termination that is triggered by dc::fatal or dc::core output.
//
// Returns true only for the first fatal path. The flag is intentionally never cleared because successful
// callers abort, _Exit, or spin for debugger attachment; later contenders must wait for process termination
// instead of generating a competing core dump.
bool claim_fatal_termination_ownership()
{
  bool expected = false;
  return fatal_termination_started.compare_exchange_strong(expected, true, std::memory_order_acq_rel,
                                                           std::memory_order_acquire);
}

struct ChannelSets
{
  std::vector<Channel*> visible_;
  std::vector<Channel*> hidden_;
  int next_index_{};
};

// Store registered debug channels behind a typed read/write lock.
//
// DebugChannels is declared in macro_ForAllDebugChannels.h because public code can iterate over
// all debug channels. The actual container type stays private to this translation unit so that the
// public libcwd header does not have to include the threadsafe implementation headers and callers
// cannot keep raw access to the registry without holding the corresponding threadsafe access object.
struct DebugChannels::Impl
{
  using channel_sets_ts = threadsafe::Unlocked<ChannelSets, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  channel_sets_ts channel_sets;
};

// A helper class to allow passing a wat to Channel::increment_and_assign_index without
// including threadsafe in a public header.
struct ChannelSetsWat
{
  DebugChannels::Impl::channel_sets_ts::wat& ref_;
  ChannelSetsWat(DebugChannels::Impl::channel_sets_ts::wat& wat) : ref_(wat) { }
};

// Return the channel registry used by ForAllDebugChannels and label lookup.
//static
DebugChannels const& DebugChannels::instance()
{
  static DebugChannels const debug_channels_instance{new DebugChannels::Impl};
  return debug_channels_instance;
}

// Return the registered channel whose label matches label as a case-insensitive prefix.
//
// When several channels match, the lexicographically largest matching label is returned so abbreviated
// lookups resolve deterministically regardless of registration order.
Channel* DebugChannels::find(char const* label) const
{
  Channel* result = nullptr;
  Impl::channel_sets_ts::rat debug_channels_r(impl->channel_sets);
  for (Channel* debug_channel : debug_channels_r->visible_)
    if (!strncasecmp(label, debug_channel->get_label(), strlen(label)))
      result = debug_channel;
  return result;
}

// Initialize channel and register it in either the public or hidden registry.
//
// The ChannelSets wat protects visible channels, hidden channels, label width updates, and next_index_.
void DebugChannels::initialize_channel(Channel& channel, char const* label, LIBCWD_TSD_PARAM,
                                       bool add_to_channel_list) const
{
  size_t label_len = strlen(label);

  if (label_len > max_label_len) // Only happens for customized channels
    DoutFatal(dc::core, "strlen(\"" << label << "\") > " << max_label_len);

  Impl::channel_sets_ts::wat channels_w(impl->channel_sets);

  const_cast<char*>(channels::dc::core.get_label())[WST_max_len] = ' ';
  const_cast<char*>(channels::dc::fatal.get_label())[WST_max_len] = ' ';
  for (Channel* debug_channel : channels_w->visible_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = ' ';
  for (Channel* debug_channel : channels_w->hidden_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = ' ';

  // MT: This is not strict thread safe because it is possible that after threads are already
  //     running, a new shared library is dlopen-ed with a new global Channel with a larger
  //     label.  However, it makes no sense to lock the *reading* of WST_max_len for the other
  //     threads because this assignment is an atomic operation.  The lock above is only needed
  //     to prefend two such simultaneously loaded libraries from causing WST_max_len to end up
  //     not maximal.
  //     When this thread exits later, there is no need to take action: the debug channel
  //     is global and can be used by any thread.  We want to keep the maximum label length as
  //     set by this channel.  Even when this debug channel is part of shared library that is
  //     being loaded by dlopen() then it won't get removed when this thread exits.
  //     (Actually, a downwards update of WST_max_len should be done by dlclose if at all).
  if (label_len > WST_max_len)
    WST_max_len = label_len;

  const_cast<char*>(channels::dc::core.get_label())[WST_max_len] = '\0';
  const_cast<char*>(channels::dc::fatal.get_label())[WST_max_len] = '\0';
  for (Channel* debug_channel : channels_w->visible_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = '\0';
  for (Channel* debug_channel : channels_w->hidden_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = '\0';

  channel.initialize(channels_w, label, label_len);

  // Turn debug channel "WARNING" on by default.
  __libcwd_tsd.off_cnt_array[channel.index()] = strncmp(channel.get_label(), "WARNING", label_len) == 0 ? -1 : 0;

  if (add_to_channel_list)
  {
    // We store debug channels in some organized order, so that the
    // order in which they appear in the ForAllDebugChannels is not
    // dependent on the order in which these global objects are
    // initialized.
    auto i = channels_w->visible_.begin();
    for (; i != channels_w->visible_.end(); ++i)
      if (strncmp((*i)->get_label(), channel.get_label(), WST_max_len) > 0)
        break;
    channels_w->visible_.insert(i, &channel);
  }
  else
    channels_w->hidden_.push_back(&channel);
}

// Initialize the built-in fatal channel state while holding the public channel registry write lock.
//
// Fatal channels are not inserted into the registry, but their label width still contributes to the
// shared WST_max_len value used when formatting all registered channel labels.
void DebugChannels::initialize_fatal_channel(FatalChannel& channel, char const* label,
                                             control_flag_t maskbit, LIBCWD_TSD_PARAM) const
{
  channel.maskbit_ = maskbit;

  size_t label_len = strlen(label);

  if (label_len > max_label_len) // Only happens for customized channels
    DoutFatal(dc::core, "strlen(\"" << label << "\") > " << max_label_len);

  Impl::channel_sets_ts::wat channels_w(impl->channel_sets);

  for (Channel* debug_channel : channels_w->visible_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = ' ';
  for (Channel* debug_channel : channels_w->hidden_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = ' ';

  // MT: See comments in DebugChannels::initialize_channel above.
  if (label_len > WST_max_len)
    WST_max_len = label_len;

  for (Channel* debug_channel : channels_w->visible_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = '\0';
  for (Channel* debug_channel : channels_w->hidden_)
    const_cast<char*>(debug_channel->get_label())[WST_max_len] = '\0';

  // clang-format off
  PRAGMA_DIAGNOSTIC_PUSH_IGNORE_stringop_overflow
  strncpy(channel.label_, label, label_len);
  PRAGMA_DIAGNOSTIC_POP
  // clang-format on
  std::memset(channel.label_ + label_len, ' ', max_label_len - label_len);
  channel.label_[WST_max_len] = '\0';
}

// Invoke callback for each registered public debug channel while holding a registry read lock.
//
// The callback receives the caller supplied data pointer unchanged. All iteration is performed while the
// read access object is alive, preventing concurrent modifications of the vector during traversal.
void DebugChannels::for_each_impl(callback_type callback, void* data) const
{
  Impl::channel_sets_ts::rat debug_channels_r(impl->channel_sets);
  for (Channel* debug_channel : debug_channels_r->visible_)
    callback(*debug_channel, data);
}

// Store registered debug objects behind a typed read/write lock.
//
// DebugObjects is declared in macro_ForAllDebugObjects.h because public code can iterate over all
// debug objects. The actual container type stays private to this translation unit so that the public
// libcwd header does not have to include the threadsafe implementation headers and callers cannot keep
// raw access to the registry without holding the corresponding threadsafe access object.
struct DebugObjects::Impl
{
  using debug_objects_ts =
      threadsafe::Unlocked<std::vector<DebugObject*>, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  debug_objects_ts debug_objects;
};

// Return the registry used by every DebugObject object in the process.
//static
DebugObjects const& DebugObjects::instance()
{
  static DebugObjects const debug_objects_instance{new DebugObjects::Impl};
  return debug_objects_instance;
}

// Register debug_object if it has not already been registered.
//
// A single write access covers both the duplicate check and the insertion so concurrent initialization
// cannot register the same debug object twice.
void DebugObjects::add_if_missing(DebugObject* debug_object) const
{
  Impl::debug_objects_ts::wat debug_objects_w(impl->debug_objects);
  if (std::find(debug_objects_w->begin(), debug_objects_w->end(), debug_object) == debug_objects_w->end())
    debug_objects_w->push_back(debug_object);
}

// Invoke callback for each registered debug object while holding a registry read lock.
//
// The callback receives the caller supplied data pointer unchanged. All iteration is performed while the
// read access object is alive, preventing concurrent modifications of the vector during traversal.
void DebugObjects::for_each_impl(callback_type callback, void* data) const
{
  Impl::debug_objects_ts::rat debug_objects_r(impl->debug_objects);
  for (DebugObject* debug_object : *debug_objects_r)
    callback(*debug_object, data);
}

} // namespace _private_
#endif // HIDE_FROM_DOXYGEN

#if CWDEBUG_DEBUG
static long WST_debug_object_init_magic = 0;

static void init_debug_object_init_magic()
{
  struct timeval rn;
  gettimeofday(&rn, NULL);
  WST_debug_object_init_magic = rn.tv_usec;
  if (!WST_debug_object_init_magic)
    WST_debug_object_init_magic = 1;
  DEBUGDEBUG_CERR("Set WST_debug_object_init_magic to " << WST_debug_object_init_magic);
}
#endif

class OutputState
{
 public:
  Buffer buffer;
  // The temporary output buffer.

  _private_::BufferStream bufferstream;
  // Our 'stringstream'.

  control_flag_t mask;
  // The previous control bits.

  char const* label;
  // The previous label.

  int err;
  // The current errno.

 public:
  OutputState(control_flag_t m, char const* l, int e) : bufferstream(&buffer), mask(m), label(l), err(e) { }
};

static inline void write_whitespace_to(std::ostream& os, unsigned int size)
{
  for (unsigned int i = size; i > 0; --i)
    os.put(' ');
}

namespace _private_ {
alignas(OutputState) static unsigned char WST_dummy_output_state[sizeof(OutputState)];
} // namespace _private_

/**
 * \fn void core_dump()
 * \ingroup chapter_core_dump
 *
 * \brief Dump core of current thread.
 *
 * <b>Example:</b>
 *
 * \code
 * Debug( core_dump() );
 * \endcode
 *
 * Normally you don't call <code>core_dump()</code> directly though.
 * Instead you'd do for example:
 *
 * \code
 * if (error_condition)
 *   DoutFatal(dc::core, "Something went wrong");
 * \endcode
 */
[[noreturn]] void core_dump()
{
  // Are we the first thread that tries to generate a core?
  if (!_private_::claim_fatal_termination_ownership())
  {
    // Another thread is already trying to generate a core dump.
    // Wait until the program terminates.
    for (;;)
      std::this_thread::sleep_for(std::chrono::hours(24));
  }
  // Another thread that races with this path will wait above while this thread generates the core.
  std::abort();
}

size_t DebugString::calculate_capacity(size_t size)
{
  size_t capacity_plus_one = default_capacity_ + 1; // For the terminating zero.
  while (size >= capacity_plus_one)
    capacity_plus_one *= 2;
  return capacity_plus_one - 1;
}

void DebugString::NS_internal_init(char const* str, size_t len)
{
  default_capacity_ = min_capacity;
  str_ = (char*)malloc((default_capacity_ = capacity_ = calculate_capacity(len)) +
                       1); // Add one for the terminating zero. LEAK46
  strncpy(str_, str, len);
  size_ = len;
  str_[size_] = 0;
}

// This is called with alloc checking off.
void DebugString::deinitialize()
{
  free(str_);
  str_ = NULL;
}

// This is called with alloc checking on (or off).
DebugString::~DebugString()
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT(
      str_ ==
      NULL); // Need to call DebugString::deinitialize() before destructor.
             // But not in the non-threaded case, see DebugObject_ThreadSpecificData::~DebugObject_ThreadSpecificData.
#endif
}

void DebugString::internal_assign(char const* str, size_t len)
{
  if (len > capacity_ || (capacity_ > default_capacity_ && len < default_capacity_))
    str_ = (char*)realloc(str_, (capacity_ = calculate_capacity(len)) + 1);
  strncpy(str_, str, len);
  size_ = len;
  str_[size_] = 0;
}

void DebugString::internal_append(char const* str, size_t len)
{
  if (size_ + len > capacity_ || (capacity_ > default_capacity_ && size_ + len < default_capacity_))
    str_ = (char*)realloc(str_, (capacity_ = calculate_capacity(size_ + len)) + 1);
  strncpy(str_ + size_, str, len);
  size_ += len;
  str_[size_] = 0;
}

void DebugString::internal_prepend(char const* str, size_t len)
{
  if (size_ + len > capacity_ || (capacity_ > default_capacity_ && size_ + len < default_capacity_))
    str_ = (char*)realloc(str_, (capacity_ = calculate_capacity(size_ + len)) + 1);
  memmove(str_ + len, str_, size_ + 1);
  strncpy(str_, str, len);
  size_ += len;
}

/**
 * \brief Reserve memory for the string in advance.
 */
void DebugString::reserve(size_t size)
{
  if (size < size_)
    return;
  LIBCWD_TSD_DECLARATION;
  default_capacity_ = min_capacity;
  str_ = (char*)realloc(str_, (default_capacity_ = capacity_ = calculate_capacity(size)) + 1);
}

void DebugString::internal_swallow(DebugString const& ds)
{
  // `this' and `ds' are both initialized.
  free(str_);
  str_ = ds.str_;
  size_ = ds.size_;
  capacity_ = ds.capacity_;
  default_capacity_ = ds.default_capacity_;
}

/** \addtogroup group_formatting */
/** \{ */

/**
 * \brief Push the current margin on a stack.
 */
void DebugObject::push_margin()
{
  LIBCWD_TSD_DECLARATION;
  DebugStringStackElement* current_margin_stack = LIBCWD_TSD_MEMBER(margin_stack);
  void* new_debug_string = malloc(sizeof(DebugStringStackElement));
  LIBCWD_TSD_MEMBER(margin_stack) = new (new_debug_string) DebugStringStackElement(LIBCWD_TSD_MEMBER(margin));
  LIBCWD_TSD_MEMBER(margin_stack)->next = current_margin_stack;
}

/**
 * \brief Pop margin from the stack.
 */
void DebugObject::pop_margin()
{
  LIBCWD_TSD_DECLARATION;
  if (!LIBCWD_TSD_MEMBER(margin_stack))
    DoutFatal(dc::core, "Calling `DebugObject::pop_margin' more often than `DebugObject::push_margin'.");
  DebugStringStackElement* next = LIBCWD_TSD_MEMBER(margin_stack)->next;
  LIBCWD_TSD_MEMBER(margin).internal_swallow(LIBCWD_TSD_MEMBER(margin_stack)->debug_string);
  free(LIBCWD_TSD_MEMBER(margin_stack));
  LIBCWD_TSD_MEMBER(margin_stack) = next;
}

/**
 * \brief Push the current marker on a stack.
 */
void DebugObject::push_marker()
{
  LIBCWD_TSD_DECLARATION;
  DebugStringStackElement* current_marker_stack = LIBCWD_TSD_MEMBER(marker_stack);
  void* new_debug_string = malloc(sizeof(DebugStringStackElement));
  LIBCWD_TSD_MEMBER(marker_stack) = new (new_debug_string) DebugStringStackElement(LIBCWD_TSD_MEMBER(marker));
  LIBCWD_TSD_MEMBER(marker_stack)->next = current_marker_stack;
}

/**
 * \brief Pop marker from the stack.
 */
void DebugObject::pop_marker()
{
  LIBCWD_TSD_DECLARATION;
  if (!LIBCWD_TSD_MEMBER(marker_stack))
    DoutFatal(dc::core, "Calling `DebugObject::pop_marker' more often than `DebugObject::push_marker'.");
  DebugStringStackElement* next = LIBCWD_TSD_MEMBER(marker_stack)->next;
  LIBCWD_TSD_MEMBER(marker).internal_swallow(LIBCWD_TSD_MEMBER(marker_stack)->debug_string);
  free(LIBCWD_TSD_MEMBER(marker_stack));
  LIBCWD_TSD_MEMBER(marker_stack) = next;
}

/** \} */

void DebugObject_ThreadSpecificData::start(DebugObject& debug_object,
                                           ChannelSetData& channel_set, LIBCWD_TSD_PARAM)
{
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
  // Initialisation of the TSD part should be done from LIBCWD_TSD_DECLARATION inside Dout et al.
  LIBCWD_ASSERT(tsd_initialized);
#endif

  // Skip `start()' for a `continued' debug output.
  // Generating the "prefix: <continued>" is already taken care
  // of while generating the "<unfinished>" (see next `if' block).
  if ((channel_set.mask & (continued_maskbit | finish_maskbit)))
  {
    current->err = errno; // Always keep the last errno as set at the start of LibcwDout()
    if (!(current->mask & continued_expected_maskbit))
    {
      std::ostream* const preferred_os = (channel_set.mask & cerr_cf) ? &std::cerr : nullptr;
      std::ostream* target_os;
      _private_::LockInterfaceBase* target_mutex;
      bool have_target;
      // Try to get the stream lock, but don't try too long.
      struct timespec const t = {0, 5000000};
      int count = 0;
      do
      {
        have_target = debug_object.ostream_state_.try_lock_os(preferred_os, &target_os, &target_mutex);
        if (!have_target) // mutex exists, but could not immediately be locked.
          nanosleep(&t, NULL);
      } while (!have_target && ++count < 40);
      DebugString const& color_off = LIBCWD_DO_TSD_MEMBER(debug_object, color_off);
      size_t color_off_size = color_off.size();
      if (have_target)
      {
        if (color_off_size > 0)
          target_os->write(color_off.c_str(), color_off_size);
        target_os->put('\n');
        if (target_mutex)
          target_mutex->unlock();
      }
      else
        debug_object.ostream_state_.write_color_off_newline(preferred_os, color_off.c_str(), color_off_size);
      char const* channame = (channel_set.mask & finish_maskbit) ? "finish" : "continued";
#if CWDEBUG_LOCATION
      DoutFatal(dc::core, "Using `dc::" << channame << "' in "
                                        << Location((char*)__builtin_return_address(0) + builtin_return_address_offset)
                                        << " without (first using) a matching `continued_cf'.");
#else
      DoutFatal(dc::core, "Using `dc::" << channame << "' without (first using) a matching `continued_cf'.");
#endif
    }
#if CWDEBUG_DEBUG
    // MT: current != _private_::WST_dummy_output_state, otherwise we didn't pass the previous if.
    LIBCWD_ASSERT(current != reinterpret_cast<OutputState*>(_private_::WST_dummy_output_state));
#endif
    current->mask = channel_set.mask; // New bits might have been added
    if ((current->mask & finish_maskbit))
      current->mask &= ~continued_expected_maskbit;
    return;
  }

  ++LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
  DEBUGDEBUG_CERR("Entering DebugObject::start(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object));

  // Is this an interrupting debug output (in the middle of a continued debug output)?
  if ((current->mask & continued_cf_maskbit) && unfinished_expected)
  {
#if CWDEBUG_DEBUG
    // MT: if current == _private_::WST_dummy_output_state then
    //     (current->mask & continued_cf_maskbit) is false and this if is skipped.
    LIBCWD_ASSERT(current != reinterpret_cast<OutputState*>(_private_::WST_dummy_output_state));
#endif
    int saved_errno = errno; // The writeto below changes errno.
    // And write out what is in the buffer till now.
    std::ostream* target_os = (channel_set.mask & cerr_cf) ? &std::cerr : nullptr;
    current->buffer.writeto(
        target_os, LIBCWD_TSD, debug_object,
        true, // This thread requests an <unfinished> because of previous, unfinished 'continued' output.
        false // Don't flush.
       , true // This output ends on a newline by itself.
       , false); // The newline is not missing as a result of nonewline_cf.
    // Truncate the buffer to its prefix and append "<continued>" to it already.
    current->buffer.restore_position();
    DebugString const& color_off = LIBCWD_DO_TSD_MEMBER(debug_object, color_off);
    size_t color_off_size = color_off.size();
    if (color_off_size > 0)
      current_bufferstream->write(color_off.c_str(), color_off_size);
    DebugString const& color_on = LIBCWD_DO_TSD_MEMBER(debug_object, color_on);
    size_t color_on_size = color_on.size();
    if (color_on_size > 0)
    {
      current_bufferstream->write("<continued>", 11); // therefore we repeat the space here.
      current_bufferstream->write(color_on.c_str(), color_on_size);
      current_bufferstream->put(' ');
    }
    else
      current_bufferstream->write("<continued> ", 12); // therefore we repeat the space here.
    current->buffer.continued();
    errno = saved_errno;
  }

  // Is this a nested debug output (the first of a series in the middle of another debug output)?
  // MT: if current == _private_::WST_dummy_output_state then
  //     start_expected is true and this if is skipped.
  if (!start_expected)
  {
    // Put current stringstream on the stack.
    output_state_stack.push(current);

    // Indent nested debug output with 4 extra spaces.
    indent += 4;

    // If the previous target was written to cerr, then
    // write this interrupting output to cerr too.
    channel_set.mask |= (current->mask & cerr_cf);
  }

  // Create a new output state.
  DEBUGDEBUG_CERR("creating new OutputState");
  current = new OutputState(channel_set.mask, channel_set.label, errno); // LEAK5: This allocation + Location.
  DEBUGDEBUG_CERR("current = " << (void*)current);
  current_bufferstream = &current->bufferstream;
  DEBUGDEBUG_CERR("OutputState created");

  // Without a new nested Dout() call, we expect to see a finish() call: The finish belonging to *this* Dout() call.
  start_expected = false;

  // If this is a `continued' debug output, then we want to print "<unfinished>" if next we see a start().
  unfinished_expected = true;

  size_t color_on_size = color_on.size();
  if (color_on_size > 0)
    current_bufferstream->write(color_on.c_str(), color_on_size);

  // Print prefix if requested.
  // Handle most common case first: no special flags set
  if (!(channel_set.mask & (noprefix_cf | nolabel_cf | blank_margin_cf | blank_label_cf | blank_marker_cf)))
  {
    current_bufferstream->write(margin.c_str(), margin.size());
    current_bufferstream->write(channel_set.label, WST_max_len);
    current_bufferstream->write(marker.c_str(), marker.size());
    write_whitespace_to(*current_bufferstream, indent);
  }
  else if (!(channel_set.mask & noprefix_cf))
  {
    if ((channel_set.mask & blank_margin_cf))
      write_whitespace_to(*current_bufferstream, margin.size());
    else
      current_bufferstream->write(margin.c_str(), margin.size());
#if !CWDEBUG_DEBUGOUTPUT
    if (!(channel_set.mask & nolabel_cf))
#endif
    {
      if ((channel_set.mask & blank_label_cf))
        write_whitespace_to(*current_bufferstream, WST_max_len);
      else
        current_bufferstream->write(channel_set.label, WST_max_len);
      if ((channel_set.mask & blank_marker_cf))
        write_whitespace_to(*current_bufferstream, marker.size());
      else
        current_bufferstream->write(marker.c_str(), marker.size());
      write_whitespace_to(*current_bufferstream, indent);
    }
  }

  if ((channel_set.mask & continued_cf_maskbit))
  {
    // If this is continued debug output, then it makes sense to remember the prefix length,
    // just in case we need indeed to output <continued> data.
    current->buffer.store_position();
  }

  --LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
  DEBUGDEBUG_CERR("Leaving DebugObject::start(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object));
}

void DebugObject_ThreadSpecificData::finish(DebugObject& debug_object,
                                            ChannelSetData& /*UNUSED, */, LIBCWD_TSD_PARAM)
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT(current != reinterpret_cast<OutputState*>(_private_::WST_dummy_output_state));
#endif
  std::ostream* target_os = (current->mask & cerr_cf) ? &std::cerr : nullptr;

  // Skip `finish()' for a `continued' debug output.
  if ((current->mask & continued_cf_maskbit) && !(current->mask & finish_maskbit))
  {
    // Allow a subsequent Dout(dc::continued (or dc::finish), ...).
    current->mask |= continued_expected_maskbit;
    if ((current->mask & continued_maskbit))
      unfinished_expected = true;
    // If the `flush_cf' control flag is set, flush the ostream at every `finish()' though.
    if (debug_object.always_flush_is_on() || (current->mask & flush_cf) != 0)
    {
      // Write buffer to ostream.
      // Flush ostream.  Note that in the case of nested debug output this `os' can be an stringstream,
      // in that case, no actual flushing is done until the debug output to the real ostream has
      // finished.
      current->buffer.writeto(
          target_os, LIBCWD_TSD, debug_object,
          false, // This thread requests <unfinished> because of previous, unfinished 'continued' output.
          true // Flush ostream after printing this.
         , false // This output does not end on a newline.
         , false); // The newline is not missing as a result of nonewline_cf.
    }
    return;
  }

  ++LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
  DEBUGDEBUG_CERR("Entering DebugObject::finish(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object));

  // Handle control flags, if any:
  if ((current->mask & error_cf))
  {
    // strerror[_r] can call malloc (in gettext()).
    char error_text_buf[512];
    char const* error_text;
#ifdef _GNU_SOURCE
    error_text = strerror_r(current->err, error_text_buf, sizeof(error_text_buf));
#else // POSIX
    if (strerror_r(current->err, error_text_buf, sizeof(error_text_buf)) == -1)
    {
      if (errno == ERANGE)
        error_text = "<libcwd: Oops, error text longer than 512 characters>";
      else
        error_text = "Unknown Error";
    }
    else
      error_text = error_text_buf;
#endif
    *current_bufferstream << ": " << strerrno(current->err) << " (" << error_text << ')';
  }
  if (!(current->mask & nonewline_cf))
  {
    DebugString const& color_off = LIBCWD_DO_TSD_MEMBER(debug_object, color_off);
    size_t color_off_size = color_off.size();
    if (color_off_size > 0)
      current_bufferstream->write(color_off.c_str(), color_off_size);
    current_bufferstream->put('\n');
  }

  // Handle control flags, if any:
  if (current->mask != 0)
  {
    if ((current->mask & (coredump_maskbit | fatal_maskbit)))
    {
      current->buffer.writeto(
          target_os, LIBCWD_TSD, debug_object,
          false, // This thread requests <unfinished> because of previous, unfinished 'continued' output.
          !__libcwd_tsd.recursive_fatal // Flush ostream after printing this when there is no recursive loop yet.
              , !(current->mask & nonewline_cf) // Whether or not this output ends on a newline.
          , true); // If the newline is missing, then it is missing because of the use of nonewline_cf.
      __libcwd_tsd.recursive_fatal = true;
      if (!_private_::claim_fatal_termination_ownership())
      {
        // Another thread is already trying to generate a core dump.
        // Wait until the program terminates.
        for (;;)
          std::this_thread::sleep_for(std::chrono::hours(24));
      }
      if ((current->mask & coredump_maskbit))
        std::abort(); // core dump.
      _Exit(254); // Exit without calling global destructors.
    }
    if ((current->mask & wait_cf))
    {
      current->buffer.writeto(
          target_os, LIBCWD_TSD, debug_object, false,
          debug_object.interactive_, !(current->mask & nonewline_cf), true);
      _private_::LockInterfaceBase* locked_mutex;
      std::ostream* locked_os;
      locked_os = debug_object.ostream_state_.get_locked_os(target_os, &locked_mutex);
      *locked_os << "(type return)";
      if (debug_object.interactive_)
      {
        *locked_os << std::flush;
        while (std::cin.get() != '\n')
          ;
      }
      if (locked_mutex)
        locked_mutex->unlock();
    }
    else
      current->buffer.writeto(target_os, LIBCWD_TSD, debug_object, false,
                              debug_object.always_flush_is_on() || (current->mask & flush_cf != 0)
                                                                      , !(current->mask & nonewline_cf)
                                                                          , true);
  }
  else
    current->buffer.writeto(
        target_os, LIBCWD_TSD, debug_object, false,
        debug_object.always_flush_is_on(), !(current->mask & nonewline_cf), true);

  DEBUGDEBUG_CERR("Deleting `current' " << (void*)current);
  control_flag_t mask = current->mask; // Keep this.
  delete current;
  DEBUGDEBUG_CERR("Done deleting `current'");

  if (start_expected)
  {
    // Ok, we're done with the last buffer.
    indent -= 4;
    output_state_stack.pop();
  }

  // Restore previous buffer as being the current one, if any.
  if (output_state_stack.size())
  {
    current = output_state_stack.top();
    DEBUGDEBUG_CERR("current = " << (void*)current);
    current_bufferstream = &current->bufferstream;
    if (debug_object.always_flush_is_on() || (mask & flush_cf != 0))
      current->mask |= flush_cf; // Propagate flush to real ostream.
  }
  else
  {
    current = reinterpret_cast<OutputState*>(
        _private_::WST_dummy_output_state); // Used (MT: read-only!) in next DebugObject::start().
    DEBUGDEBUG_CERR("current = " << (void*)current);
    current_bufferstream = NULL;
  }

  start_expected = true;
  unfinished_expected = false;

  --LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
  DEBUGDEBUG_CERR("Leaving DebugObject::finish(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object));
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winfinite-recursion"
void DebugObject_ThreadSpecificData::fatal_finish(DebugObject& debug_object,
                                                  ChannelSetData& channel_set, LIBCWD_TSD_PARAM)
{
  finish(debug_object, channel_set, LIBCWD_TSD);
  DoutFatal(dc::core,
            "Don't use `DoutFatal' together with `continued_cf', use `Dout' instead.  (This message can also occur "
            "when using DoutFatal correctly but from the constructor of a global object).");
  _Exit(1); // This is never reached, but g++ 3.3.x -O2 thinks it is.
}
#pragma clang diagnostic pop

int DebugObject::s_index_count_ = 0;

bool DebugObject::NS_init(LIBCWD_TSD_PARAM)
{
  if (being_initialized_)
    return false;

  ST_initialize_globals(LIBCWD_TSD); // Use the constructor of DebugObject to initiate
                                     // the global initialization of libcwd.

  if (initialized_)
    return true;

  being_initialized_ = true;

  unfinished_oss_ = NULL;

#if CWDEBUG_DEBUG
  if (!WST_debug_object_init_magic)
    init_debug_object_init_magic();
  init_magic_ = WST_debug_object_init_magic;
  DEBUGDEBUG_CERR("Set init_magic_ to " << init_magic_);
  DEBUGDEBUG_CERR("Setting initialized_ to true");
#endif

  _private_::DebugObjects::instance().add_if_missing(this);
  new (_private_::WST_dummy_output_state)
      OutputState(0, channels::dc::debug.get_label(), 0); // Leaks 24 bytes of memory
  index_ = s_index_count_++;
#if CWDEBUG_DEBUGT
  LIBCWD_ASSERT(!_private_::WST_multi_threaded.load(
      std::memory_order_relaxed)); // Only the first thread should be initializing DebugObject objects.
#endif
  LIBCWD_ASSERT(__libcwd_tsd.debug_object_array[index_] == NULL);
  DebugObject_ThreadSpecificData& tsd(*(__libcwd_tsd.debug_object_array[index_] = new DebugObject_ThreadSpecificData));
  tsd.init();

#if CWDEBUG_DEBUGOUTPUT
  LIBCWD_TSD_MEMBER_OFF = -1; // Print as much debug output as possible right away.
#else
  LIBCWD_TSD_MEMBER_OFF = 0; // Don't print debug output till the REAL initialization of the debug system
                             // has been performed (ie, the _application_ start (don't confuse that with
                             // the constructor - which does nothing)).
#endif
  DEBUGDEBUG_CERR("DebugObject::NS_init(), _off set to " << LIBCWD_TSD_MEMBER_OFF);

  set_ostream(&std::cerr); // Write to std::cerr by default.
  interactive_ = true; // and thus we're interactive.

  being_initialized_ = false;
  initialized_ = true;
  return true; // Success.
}

void DebugObject_ThreadSpecificData::init()
{
  DEBUGDEBUG_CERR("Entering DebugObject_ThreadSpecificData::init (this == " << (void*)this << ")");
  start_expected = true; // Of course, we start with expecting the beginning of a debug output.
  unfinished_expected = false;

  // `current' needs to be non-zero (saving us a check in start()) and
  // current->mask needs to be 0 to avoid a crash in start():
  current = reinterpret_cast<OutputState*>(_private_::WST_dummy_output_state);
  DEBUGDEBUG_CERR("current = " << (void*)current);
  current_bufferstream = NULL;
  output_state_stack.init();
  continued_stack.init();
  color_on.NS_internal_init("", 0);
  color_off.NS_internal_init("", 0);
  margin.NS_internal_init("", 0);
  marker.NS_internal_init(": ", 2); // LEAK7
#if CWDEBUG_DEBUGOUTPUT
  first_time = true;
#endif
  off_count = 0;
  margin_stack = NULL;
  marker_stack = NULL;
  indent = 0;

  tsd_initialized = true;
  DEBUGDEBUG_CERR("Leaving DebugObject_ThreadSpecificData::init (this == " << (void*)this << "); &tsd_initialized == "
                                                                           << (void*)&tsd_initialized);
}

namespace _private_ {
void debug_tsd_init(LIBCWD_TSD_PARAM)
{
  // clang-format off
  ForAllDebugObjects(
    LIBCWD_ASSERT(__libcwd_tsd.debug_object_array[(debugObject).index_] == NULL);
    DebugObject_ThreadSpecificData& tsd(*(__libcwd_tsd.debug_object_array[(debugObject).index_] = new DebugObject_ThreadSpecificData));
    tsd.init();
    LIBCWD_DO_TSD_MEMBER_OFF(debugObject) = 0;
  );
  // clang-format on
}
} // namespace _private_

DebugObject_ThreadSpecificData::~DebugObject_ThreadSpecificData()
{
  // In the non-threaded case we do not want to deinitialize these because they
  // might still be needed if dc::elfutils is turned on and
  // the destructor of some global object that is destructed *after* libcw_do
  // is deleting (or allocating) memory.

  color_on.deinitialize();
  color_off.deinitialize();
  margin.deinitialize();
  marker.deinitialize();
  if (!tsd_initialized) // Skip the rest when it wasn't initialized.
    return;
  // Sanity checks:
  if (continued_stack.size())
    DoutFatal(dc::core | cerr_cf,
              "Destructing DebugObject_ThreadSpecificData with a non-empty continued_stack (missing dc::finish?)");
  if (output_state_stack.size())
    DoutFatal(dc::core | cerr_cf, "Destructing DebugObject_ThreadSpecificData with a non-empty output_state_stack");
}

/**
 * \fn Channel* find_channel(char const* label)
 * \ingroup group_special
 *
 * \brief Find %debug channel with label \a label.
 *
 * \return A pointer to the %debug channel object whose name starts with \a label.
 * &nbsp;If there is more than one such %debug %channel, the object with the lexicographically
 * largest name is returned.&nbsp; When no %debug channel could be found, NULL is returned.
 */
Channel* find_channel(char const* label)
{
  Channel* tmp = nullptr;
  tmp = _private_::DebugChannels::instance().find(label);
  return tmp;
}

/**
 * \fn void list_channels_on(DebugObject& debug_object)
 * \ingroup group_special
 *
 * \brief List all %debug %channels to a given %debug object.
 *
 * <b>Example:</b>
 *
 * \code
 * Dout( list_channels_on(libcw_do) );   // libcw_do is the (default) debug object of libcwd.
 * \endcode
 *
 * Example of output:
 *
 * \exampleoutput <PRE>
 * DEBUG   : Disabled
 * NOTICE  : Enabled
 * WARNING : Enabled
 * SYSTEM  : Enabled
 * LLISTS  : Disabled
 * KERNEL  : Disabled
 * IO      : Disabled
 * FOO     : Enabled
 * BAR     : Enabled</PRE>
 * \endexampleoutput
 *
 * Where FOO and BAR are \link preparation user defined channels \endlink in this example.
 */
void list_channels_on(DebugObject& debug_object)
{
  LIBCWD_TSD_DECLARATION;
  if (LIBCWD_DO_TSD_MEMBER_OFF(debug_object) < 0)
  {
    _private_::DebugChannels::instance().for_each([&](Channel& debugChannel) {
      LibcwDoutScopeBegin(LIBCWD_DEBUGCHANNELS, debug_object, dc::always | noprefix_cf);
      LibcwDoutStream.write(LIBCWD_DO_TSD_MEMBER(debug_object, margin).c_str(),
                            LIBCWD_DO_TSD_MEMBER(debug_object, margin).size());
      LibcwDoutStream.write(debugChannel.get_label(), WST_max_len);
      if (debugChannel.is_on(LIBCWD_TSD))
        LibcwDoutStream.write(": Enabled", 9);
      else
        LibcwDoutStream.write(": Disabled", 10);
      LibcwDoutScopeEnd;
    });
  }
}

void Channel::NS_initialize(char const* label, LIBCWD_TSD_PARAM, bool add_to_channel_list)
{
  // This is pretty much identical to FatalChannel::FatalChannel().

  if (initialized_)
    return; // Already initialized.

  DEBUGDEBUG_CERR("Entering `Channel::NS_initialize(\"" << label << "\")'");

  _private_::DebugChannels::instance().initialize_channel(*this, label, LIBCWD_TSD, add_to_channel_list);

  DEBUGDEBUG_CERR("Leaving `Channel::NS_initialize(\"" << label << "\")");
}

void FatalChannel::NS_initialize(char const* label, control_flag_t maskbit, LIBCWD_TSD_PARAM)
{
  // This is pretty much identical to Channel::NS_initialize().

  if (maskbit_)
    return; // Already initialized.

  DEBUGDEBUG_CERR("Entering `FatalChannel::NS_initialize(\"" << label << "\")'");

  _private_::DebugChannels::instance().initialize_fatal_channel(*this, label, maskbit, LIBCWD_TSD);

  DEBUGDEBUG_CERR("Leaving `FatalChannel::NS_initialize(\"" << label << "\")");
}

void ContinuedChannel::NS_initialize(control_flag_t maskbit)
{
  if (!maskbit_)
    maskbit_ = maskbit;
}

void Channel::initialize(_private_::ChannelSetsWat wat, char const* label, size_t label_len)
{
  index_ =
      ++wat.ref_
            ->next_index_; // Don't use index 0, it is used to make sure that uninitialized channels appear to be off.
  // clang-format off
  PRAGMA_DIAGNOSTIC_PUSH_IGNORE_stringop_overflow
  strncpy(label_, label, label_len);
  PRAGMA_DIAGNOSTIC_POP
  // clang-format on
  std::memset(label_ + label_len, ' ', max_label_len - label_len);
  label_[WST_max_len] = '\0';
  initialized_ = true;
}

/**
 * \brief Turn this channel off.
 *
 * \sa on()
 */
void Channel::off()
{
  LIBCWD_TSD_DECLARATION;
  __libcwd_tsd.off_cnt_array[index_] += 1;
}

/**
 * \brief Cancel one call to `off()`.
 *
 * The channel is turned on when `on()` is called as often as `off()` was called before.
 */
void Channel::on()
{
  LIBCWD_TSD_DECLARATION;
  if (__libcwd_tsd.off_cnt_array[index_] == -1)
    DoutFatal(dc::core, "Calling Channel::on() more often than Channel::off()");
  __libcwd_tsd.off_cnt_array[index_] -= 1;
}

//----------------------------------------------------------------------------------------
// Handle continued channel flags and update `off_count' and `continued_stack'.
//

namespace _private_ {
void print_pop_error()
{
  DoutFatal(dc::core,
            "Using \"dc::finish\" without corresponding \"continued_cf\" or "
            "calling the Dout(dc::finish, ...) more often than its corresponding "
            "Dout(dc::channel|continued_cf, ...).  Note that the wrong \"dc::finish\" doesn't "
            "have to be the one that we core dumped on, if two or more are nested.");
}
} // namespace _private_

ContinuedChannelSet& ChannelSet::operator|(continued_cf_nt)
{
#if CWDEBUG_DEBUG
  DEBUGDEBUG_CERR("continued_cf detected");
  if (!debug_object_tsd_ptr || !debug_object_tsd_ptr->tsd_initialized)
  {
    if (debug_object_tsd_ptr)
      DEBUGDEBUG_CERR("&debug_object_tsd_ptr->tsd_initialized == " << (void*)&debug_object_tsd_ptr->tsd_initialized);
    FATALDEBUGDEBUG_CERR("Don't use DoutFatal together with continued_cf, use Dout instead.");
    core_dump();
  }
#endif
  mask |= continued_cf_maskbit;
  if (!on)
  {
    ++(debug_object_tsd_ptr->off_count);
    DEBUGDEBUG_CERR("Channel is switched off. Increased off_count to " << debug_object_tsd_ptr->off_count);
  }
  else
  {
    debug_object_tsd_ptr->continued_stack.push(debug_object_tsd_ptr->off_count);
    DEBUGDEBUG_CERR("Channel is switched on. Pushed off_count ("
                    << debug_object_tsd_ptr->off_count << ") to stack (size now "
                    << debug_object_tsd_ptr->continued_stack.size() << ") and set off_count to 0");
    debug_object_tsd_ptr->off_count = 0;
  }
  return *(reinterpret_cast<ContinuedChannelSet*>(this));
}

ContinuedChannelSet& ChannelSetBootstrap::operator|(ContinuedChannel const& cdc)
{
#if CWDEBUG_DEBUG
  if ((cdc.get_maskbit() & continued_maskbit))
    DEBUGDEBUG_CERR("dc::continued detected");
  else
    DEBUGDEBUG_CERR("dc::finish detected");
#endif

  if ((on = !debug_object_tsd_ptr->off_count))
  {
    DEBUGDEBUG_CERR("Channel is switched on (off_count is 0)");
    debug_object_tsd_ptr->current->mask |= cdc.get_maskbit(); // We continue with the current channel
    mask = debug_object_tsd_ptr->current->mask;
    label = debug_object_tsd_ptr->current->label;
    if (cdc.get_maskbit() == finish_maskbit)
    {
      debug_object_tsd_ptr->off_count = debug_object_tsd_ptr->continued_stack.top();
      debug_object_tsd_ptr->continued_stack.pop();
      DEBUGDEBUG_CERR("Restoring off_count to " << debug_object_tsd_ptr->off_count << ". Stack size now "
                                                << debug_object_tsd_ptr->continued_stack.size());
    }
  }
  else
  {
    DEBUGDEBUG_CERR("Channel is switched off (off_count is " << debug_object_tsd_ptr->off_count << ')');
    if (cdc.get_maskbit() == finish_maskbit)
    {
      DEBUGDEBUG_CERR("` decrementing off_count with 1");
      --(debug_object_tsd_ptr->off_count);
    }
  }
  return *reinterpret_cast<ContinuedChannelSet*>(this);
}

namespace _private_ {

void assert_fail(char const* expr, char const* file, int line, char const* function)
{
#if CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION;
  if (__libcwd_tsd.recursive_assert)
  {
    if (!__libcwd_tsd.recursive_assert)
    {
      Debug(libcw_do.get_ostream()->flush());
    }
    FATALDEBUGDEBUG_CERR(file << ':' << line << ": " << function << ": Assertion `" << expr << "' failed.\n");
    core_dump();
  }
  __libcwd_tsd.recursive_assert = true;
#endif
  DoutFatal(dc::core, file << ':' << line << ": " << function << ": Assertion `" << expr << "' failed.\n");
}

} // namespace _private_

void DebugObject::force_on(DebugObject::OnOffState& state)
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUG
  if (!NS_init(LIBCWD_TSD))
    DoutFatal(dc::core, "Calling DebugObject::NS_init recursively from DebugObject::force_on");
#else
  [[maybe_unused]] bool success = NS_init(LIBCWD_TSD);
#endif
  state.off_cnt = LIBCWD_TSD_MEMBER_OFF;
#if CWDEBUG_DEBUGOUTPUT
  state.first_time = LIBCWD_TSD_MEMBER(first_time);
#endif
  LIBCWD_TSD_MEMBER_OFF = -1; // Turn object on.
}

void DebugObject::restore(DebugObject::OnOffState const& state)
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGOUTPUT
  if (state.first_time != LIBCWD_TSD_MEMBER(first_time)) // state.first_time && !first_time.
    core_dump(); // on() was called without first a call to off().
#endif
  if (LIBCWD_TSD_MEMBER_OFF != -1)
    core_dump(); // off() and on() where called and not in equal pairs.
  LIBCWD_TSD_MEMBER_OFF = state.off_cnt; // Restore.
}

void Channel::force_on(Channel::OnOffState& state, char const* label)
{
  LIBCWD_TSD_DECLARATION;
  NS_initialize(label, LIBCWD_TSD, true);
  int& off_cnt(__libcwd_tsd.off_cnt_array[index_]);
  state.off_cnt = off_cnt;
  off_cnt = -1; // Turn channel on.
}

void Channel::restore(Channel::OnOffState const& state)
{
  LIBCWD_TSD_DECLARATION;
  int& off_cnt(__libcwd_tsd.off_cnt_array[index_]);
  if (off_cnt != -1)
    core_dump(); // off() and on() where called and not in equal pairs.
  off_cnt = state.off_cnt; // Restore.
}

/**
 * \brief Set output device (single threaded applications).
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object (default is `std::cerr`).
 * For use in single threaded applications only.
 */
void DebugObject::set_ostream(std::ostream* os)
{
  if (_private_::WST_multi_threaded.load(std::memory_order_relaxed))
#if CWDEBUG_LOCATION
    Dout(dc::warning, Location((char*)__builtin_return_address(0) + builtin_return_address_offset)
                          << ": You should passing a locking mechanism to `set_ostream' for the ostream (see "
                             "documentation/group__group__destination.html)");
#else
    DoutFatal(dc::core,
              "You must pass a locking mechanism to `set_ostream' for the ostream (see "
              "documentation/group__group__destination.html)");
#endif
#if CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT(LIBCWD_TSD_MEMBER(tsd_initialized));
#endif
  ostream_state_.set_ostream(os);
}

// This flag should be set to true at the very top of main (before any threads are created, or any location look ups are
// needed).
constinit std::atomic<bool> s_main_reached{false};

void internal_main_reached()
{
  s_main_reached.store(true, std::memory_order_relaxed);
}

bool test_main_reached()
{
  return s_main_reached.load(std::memory_order_relaxed);
}

} // namespace libcwd

// This can be used in configure to see if libcwd exists.
extern "C" char const* const __libcwd_version = VERSION;

// The following functions can be invoked from gdb directly.

namespace libcwd {
namespace _private_ {
extern void demangle_symbol(char const* in, std::string& out);
} // namespace _private_
} // namespace libcwd
