#pragma once

#ifndef CWDEBUG

// No need to document this.  See https://carlowood.github.io/libcwd/ for more info.
/// @cond Doxygen_Suppress

#include <iostream>
#include <mutex>
#include <cstdlib>              // std::exit, EXIT_FAILURE

#define Debug(x) do { } while(0)
#define Dout(a, ...) do { } while(0)
#define DoutEntering(a, ...)
#define DoutFatal(a, ...) LibcwDoutFatal(::std, , a, __VA_ARGS__)
#define ForAllDebugChannels(STATEMENT)
#define ForAllDebugObjects(STATEMENT)
#define LibcwDebug(dc_namespace, x)
#define LibcwDout(a, b, c, ...)
#define LibcwDoutFatal(a, b, c, ...) do { ::std::cerr << __VA_ARGS__ << ::std::endl; ::std::exit(EXIT_FAILURE); } while (1)
#define NEW(x) new x
#define CWDEBUG_LOCATION 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGT 0

/// @endcond

/// Remove arguments of these macros when CWDEBUG is not defined.
#define CWDEBUG_ONLY(...)
#define COMMA_CWDEBUG_ONLY(...)

#ifndef NDEBUG
/// Define this macro as 1 when either CWDEBUG is defined or NDEBUG is not defined, otherwise as 0.
#define CW_DEBUG 1

#include <cassert>
#include <atomic>

#define ASSERT(x) assert(x)
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#include <utility>
#define AI_NEVER_REACHED do { ::std::unreachable(); } while(0);
#else
#define AI_NEVER_REACHED __builtin_unreachable();
#endif
#define AI_REACHED_ONCE do { \
  static ::std::atomic_flag s_reached = ATOMIC_FLAG_INIT; assert(!s_reached.test_and_set(::std::memory_order_relaxed)); \
} while(0)

#else
#define CW_DEBUG 0
#define ASSERT(x) do { } while(0)
#define AI_NEVER_REACHED __builtin_unreachable();
#define AI_REACHED_ONCE do { } while(0)

#endif

#else // CWDEBUG

#include <ext/stdio_filebuf.h>  // __gnu_cxx::stdio_filebuf.
#include <cwds/config.h>        // Our generated config, to get NAMESPACE_DEBUG.

/// Define this macro as 1 when either CWDEBUG is defined or NDEBUG is not defined, otherwise as 0.
#define CW_DEBUG 1

/// Assert @a x, if debugging is turned on.
#define ASSERT(x) LIBCWD_ASSERT(x)
#define AI_NEVER_REACHED do { LIBCWD_ASSERT(false); __builtin_unreachable(); } while(0);
#define AI_REACHED_ONCE do { static std::atomic_flag s_reached = ATOMIC_FLAG_INIT; LIBCWD_ASSERT(!s_reached.test_and_set(std::memory_order_relaxed)); } while(0)

/// Insert debug code, only when debugging.
#define CWDEBUG_ONLY(...) __VA_ARGS__

/// Insert a comma followed by debug code, only when debugging.
#define COMMA_CWDEBUG_ONLY(...) , __VA_ARGS__

// The user should set NAMESPACE_DEBUG to a (nested) namespace name.
#ifndef NAMESPACE_DEBUG
#define NAMESPACE_DEBUG debug
#endif

// I can not think of a reason that the user needs to define this; but we'll keep the #ifndef, so that that is possible.
#ifndef NAMESPACE_DEBUG_START
#define NAMESPACE_DEBUG_START namespace NAMESPACE_DEBUG {
#define NAMESPACE_DEBUG_END }
#endif

#ifndef NAMESPACE_CHANNELS
#define NAMESPACE_CHANNELS channels
#endif

#ifndef DEBUGCHANNELS
/**
 * The namespace in which the @c dc namespace is declared.
 *
 * <A HREF="https://carlowood.github.io/libcwd/">Libcwd</A> demands that this macro is defined
 * before <libcwd/debug.h> is included and must be the name of the namespace containing
 * the @c dc (Debug Channels) namespace.
 *
 * @sa debug::channels::dc
 */
#define DEBUGCHANNELS ::NAMESPACE_DEBUG::NAMESPACE_CHANNELS
#endif

#ifndef LIBCWD_USING_OSTREAM_PRELUDE
struct MakeLIBCWD_USING_OSTREAM_PRELUDEHappy;
namespace libcwd::ostream_operators {
void operator<<(std::same_as<MakeLIBCWD_USING_OSTREAM_PRELUDEHappy> auto, int);
} // namespace libcwd::ostream_operators
#define LIBCWD_USING_OSTREAM_PRELUDE using ::libcwd::ostream_operators::operator<<;
#endif

#include <libcwd/debug.h>
#include <libcwd/char2str.h>

// Nested namespace definitions are already a part of C++17.
#define NAMESPACE_DEBUG_CHANNELS_START namespace NAMESPACE_DEBUG::NAMESPACE_CHANNELS::dc {
#define NAMESPACE_DEBUG_CHANNELS_END }

namespace libcwd {

enum thread_init_t {
  thread_init_default,
  from_rcfile,
  copy_from_main,
  debug_off
};

} // namespace libcwd

#include <atomic>       // atomic_bool
#include <utility>

/// Debug specific code.
NAMESPACE_DEBUG_START

void init();                                                                            // Initialize debugging code, called once from main.
extern libcwd::thread_init_t thread_init_default;
void init_thread(std::string thread_name = "", libcwd::thread_init_t thread_init = libcwd::thread_init_default);      // Initialize debugging code, called once for each thread.
extern std::atomic_bool threads_created;

/**
 * Debug Channels (dc) namespace.
 *
 * @sa debug::channels::dc
 */
namespace NAMESPACE_CHANNELS {

/// The namespace containing the actual debug channels.
namespace dc {
using namespace libcwd::channels::dc;
using libcwd::Channel;

// Add the declaration of new debug channels here
// and add their definition in a custom debug.cpp file.

} // namespace dc
} // namespace NAMESPACE_CHANNELS

#if CWDEBUG_LOCATION
std::string call_location(void const* return_addr);
#endif
bool being_traced();

/**
 * Interface for marking scopes with indented debug output.
 *
 * Creation of the object increments the debug indentation. Destruction
 * of the object automatically decrements the indentation again.
 */
struct Indent
{
  /// The extra number of spaces that were added to the indentation.
  int M_indent;

  /// Construct an Indent object.
  explicit Indent(int indent) : M_indent(indent) { if (M_indent > 0) libcwd::libcw_do.inc_indent(M_indent); }

  /// Destructor.
  ~Indent() { if (M_indent > 0) libcwd::libcw_do.dec_indent(M_indent); }
};

/**
 * Interface for marking scopes with a marker character.
 *
 * Creation of the object appends the character and a space to
 * the current marker after first adding the current indentation
 * to it as spaces, and sets the indentation to zero. Destruction
 * restores things again.
 */
struct Mark
{
  /// The old indentation.
  int M_indent;

  /// Construct a Mark object.
  explicit Mark(char m = '|') : M_indent(libcwd::libcw_do.get_indent())
  {
    libcwd::libcw_do.push_marker();
    libcwd::libcw_do.marker().append(std::string(M_indent, ' ') + m + ' ');
    // This is basically a decrement of M_indent.
    libcwd::libcw_do.set_indent(0);
  }
  explicit Mark(char const* utf8_m) : M_indent(libcwd::libcw_do.get_indent())
  {
    libcwd::libcw_do.push_marker();
    libcwd::libcw_do.marker().append(std::string(M_indent, ' ') + utf8_m + ' ');
    // This is basically a decrement of M_indent.
    libcwd::libcw_do.set_indent(0);
  }
#ifdef __cpp_char8_t
  explicit Mark(char8_t const* utf8_m) : Mark(reinterpret_cast<char const*>(utf8_m)) { }
#endif

  /// Destructor.
  ~Mark() { end(); }

  void end()
  {
    if (M_indent != -1)
    {
      libcwd::libcw_do.pop_marker();
      // Restore indentation relative to possible other indentation increments that happened in the mean time.
      libcwd::libcw_do.inc_indent(M_indent);
      // Mark that end() was already called.
      M_indent = -1;
    }
  }
};

void ignore_being_traced();

#if __cplusplus >= 202002L      // Only add this when C++20 is supported.

template <typename T>
concept ConceptAlwaysFalse = false;

template <ConceptAlwaysFalse T>
bool static_print(T&& = {})
{
  return false;
};

// Usage:
//
// DEBUG_STATIC_PRINT_TYPE(decltype(Class::element));
//
#define DEBUG_STATIC_PRINT_TYPE(type) \
  auto __dummy = NAMESPACE_DEBUG::static_print<decltype(TopPosition::v)>();

#endif // __cplusplus >= 202002L

NAMESPACE_DEBUG_END

NAMESPACE_DEBUG_CHANNELS_START
extern Channel system;
NAMESPACE_DEBUG_CHANNELS_END

/// A debug streambuf that prints characters written to it with a green background.
class DebugBuf : public std::streambuf
{
  public:
    DebugBuf() { Dout(dc::notice|continued_cf, ""); setp(0, 0); }
    ~DebugBuf() { Dout(dc::finish, ""); }

    /// Implement std::streambuf::overflow.
    int_type overflow(int_type c = traits_type::eof()) override
    {
      if (c != traits_type::eof())
      {
        if (c == '\n')
        {
          Dout(dc::finish, "\e[42m\\n\e[0m");
          Dout(dc::notice|continued_cf, "");
        }
        else
        {
          Dout(dc::continued, "\e[42m" << (char)c << "\e[0m");
        }
      }
      return c;
    }
};

/// A debug streambuf that prints characters written to it to a given debug channel.
class DebugStreamBuf : public std::streambuf
{
  private:
    libcwd::Channel const& m_debug_channel;

  public:
    DebugStreamBuf(libcwd::Channel const& debug_channel) : m_debug_channel(debug_channel)
    {
      Dout(debug_channel|continued_cf, "");
      setp(0, 0);
    }

    ~DebugStreamBuf()
    {
      Dout(dc::finish, "");
    }

    /// Implement std::streambuf::overflow.
    int_type overflow(int_type c = traits_type::eof()) override
    {
      if (c != traits_type::eof())
      {
        if (c == '\n')
        {
          Dout(dc::finish, "\\n");
          Dout(m_debug_channel|continued_cf, "");
        }
        else
        {
          Dout(dc::continued, char2str(c));
        }
      }
      return c;
    }
};

/// A class that wraps a POSIX pipe(2). Helper class for DebugPipedOStringStream.
class HelperPipeFDs
{
 private:
  int m_pipefd[2];

 protected:
  HelperPipeFDs();

  int fd_out() const { return m_pipefd[0]; }  // The read end of the pipe.
  int fd_in() const { return m_pipefd[1]; }   // The write end of the pipe.
};

/**
 * A class that wraps two __gnu_cxx::stdio_filebuf<char>'s.
 *
 * Helper class for DebugPipedOStringStream.
 */
struct HelperPipeBufs : public HelperPipeFDs
{
 private:
  __gnu_cxx::stdio_filebuf<char> m_obuf;  // Read from ostream, write to pipe.
  __gnu_cxx::stdio_filebuf<char> m_ibuf;  // Read from pipe write to istream.

 protected:
  HelperPipeBufs() : m_obuf(fd_in(), std::ios::out), m_ibuf(fd_out(), std::ios::in) { }

  std::streambuf* obuf() { return &m_obuf; }
  std::streambuf* ibuf() { return &m_ibuf; }

 public:
  /// Flush and close write-end of pipe. Unblocks DebugPipedOStringStream::str().
  void close() { m_obuf.close(); }
};

class DebugPipedOStringStream : public HelperPipeBufs, public std::ostream
{
 public:
  DebugPipedOStringStream() : std::ostream(obuf()) { }

  /// Read blocking from read-end of pipe until EOF. Call close() (after writing) to unblock.
  std::string str();
};

namespace libcwd {
extern std::mutex cout_mutex;
} // namespace libcwd

/**
 * Debugging macro.
 *
 * Print "Entering " << @a data to channel @a cntrl and increment
 * debugging output indentation until the end of the current scope.
 */
#define DoutEntering(cntrl, ...)                                                \
  int __cwds_debug_indentation = 2;                                             \
  LibcwDoutScopeBegin(DEBUGCHANNELS, ::libcwd::libcw_do, cntrl)                 \
  LibcwDoutStream << "Entering " << __VA_ARGS__;                                \
  LibcwDoutScopeEnd;                                                            \
  NAMESPACE_DEBUG::Indent __cwds_debug_indent(__cwds_debug_indentation);

#ifdef __cpp_fold_expressions

// Allow printing of template parameter packs.
//
// Usage: Dout(dc::notice, join(" + ", args...));
//
// Which would print, for three arguments: "arg0 + arg1 + arg2".
//

#include <tuple>
#include <type_traits>

template<typename ...Args>
struct Join
{
  char const* m_separator;
  std::tuple<Args const&...> m_args;
  Join(char const* separator, Args const&... args) : m_separator(separator), m_args(args...) { }
  Join(char const* separator, std::tuple<Args const&...>&& args) : m_separator(separator), m_args(std::move(args)) { }
  template<size_t ...I> void print_on(std::ostream& os, std::index_sequence<I...>);
};

namespace ostream_serializer_catch_all {

template<typename, typename = std::ostream&>
struct has_ostream_serializer : std::false_type
{
};

template<typename T>
struct has_ostream_serializer<T, decltype(std::cout << std::declval<T>())> : std::true_type
{
};

template<typename T>
auto operator<<(std::ostream& os, T const&) -> typename std::enable_if<!has_ostream_serializer<T>::value, std::ostream&>::type
{
  os.write("¿￼ ?", 4);
  return os;
}

} // namespace ostream_serializer_catch_all

template<typename ...Args>
template<size_t ...I>
void Join<Args...>::print_on(std::ostream& os, std::index_sequence<I...>)
{
  using ostream_serializer_catch_all::operator<<;
  (..., (os << (I == 0 ? "" : m_separator) << std::get<I>(m_args)));
}

template<typename ...Args>
std::ostream& operator<<(std::ostream& os, Join<Args...> comm)
{
  comm.print_on(os, std::make_index_sequence<sizeof...(Args)>());
  return os;
}

template<typename ...Args>
Join<Args...> join(char const* separator, Args const&... args)
{
  return { separator, args... };
}

template<typename ...Args>
Join<Args...> join_from_tuple(char const* separator, std::tuple<Args...>&& args)
{
  return { separator, std::move(args) };
}

template<typename ...Args>
Join<char const*, Args...> join_more(char const* separator, Args const&... args)
{
  // We must use a static here because the tuple binds with a const& (aka, stores a char const* const& to empty_prefix).
  // Using a string literal directly results in a reference to a temporary char const* pointing to the char[1] of the literal.
  // Thanks to aschepler for this analysis (see https://stackoverflow.com/a/57342183/1487069).
  static char const* const empty_prefix = "";
  return { separator, empty_prefix, args... };
}

#endif // __cpp_fold_expressions

#endif // CWDEBUG

#if CW_DEBUG

#define DEBUG_ONLY(...) __VA_ARGS__
#define COMMA_DEBUG_ONLY(...) , __VA_ARGS__

#else

#define DEBUG_ONLY(...)
#define COMMA_DEBUG_ONLY(...)

#endif
