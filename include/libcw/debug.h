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

#ifndef LIBCW_DEBUG_H
#define LIBCW_DEBUG_H

#ifndef LIBCW_SYS_H
#error "You need to #include <libcw/sys.h> at the top of every source file."
#endif

#ifndef CWDEBUG
// If you run into this error then you included <libcw/debug.h> while the macro CWDEBUG was not defined.
// Doing so would cause the compilation of your application to fail on machines that do not have libcwd
// installed.  Instead you should use:
// #include "debug.h"
// and add a file debug.h to your applications distribution.  Please see the the example-project that
// comes with the source code of libcwd (or is included in the documentation that comes with the rpm
// (ie: /usr/doc/libcwd-1.0/example-project) a description of the content of "debug.h".
// Note: CWDEBUG should be defined on the compiler commandline, for example: g++ -DCWDEBUG ...
#error "You are including <libcw/debug.h> while CWDEBUG is not defined.  See the comments in this header file for more information."
#endif

#include <libcw/debug_config.h>

RCSTAG_H(debug, "$Id$")

#ifdef CWDEBUG
#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::libcw::debug::channels
#endif
#define UNUSED_UNLESS_DEBUG(x) x
#include <cassert>
#define ASSERT(x) assert(x)
#define LibcwDebug(dc_namespace, x) do { using namespace ::libcw::debug; using namespace dc_namespace; { x; } } while(0)
#define Debug(x) LibcwDebug(DEBUGCHANNELS, x)
#define __Debug(x) LibcwDebug(::libcw::debug::channels, x)
#define ForAllDebugObjects(STATEMENT) \
       for( ::libcw::debug::debug_objects_ct::iterator __libcw_i(::libcw::debug::debug_objects().begin()); __libcw_i != ::libcw::debug::debug_objects().end(); ++__libcw_i) \
       { \
         using namespace ::libcw::debug; \
	 using namespace DEBUGCHANNELS; \
         debug_ct& debugObject(*(*__libcw_i)); \
	 { STATEMENT; } \
       }
#define ForAllDebugChannels(STATEMENT) \
       for( ::libcw::debug::debug_channels_ct::iterator __libcw_i(::libcw::debug::debug_channels().begin()); __libcw_i != ::libcw::debug::debug_channels().end(); ++__libcw_i) \
       { \
         using namespace ::libcw::debug; \
	 using namespace DEBUGCHANNELS; \
         channel_ct& debugChannel(*(*__libcw_i)); \
	 { STATEMENT; } \
       }
#else
#ifdef __cplusplus
#define UNUSED_UNLESS_DEBUG(x) /**/
#else
#define UNUSED_UNLESS_DEBUG(x) unused__##x
#endif
#define ASSERT(x)
#define LibcwDebug(dc_namespace, x)
#define Debug(x)
#define __Debug(x)
#define ForAllDebugObjects(STATEMENT)
#define ForAllDebugChannels(STATEMENT)
#endif

#ifdef CWDEBUG
#include <iostream>
#include <vector>
#include <string>
#ifdef DEBUGDEBUG
#include <libcw/debugdebugcheckpoint.h>
#endif

namespace libcw {
  namespace debug {

//=============================================================================
// Definitions

// The maximum number of characters that are allowed in a debug channel label.
unsigned short const max_label_len = 16;

// Define the type that is used for control flags and control flag mask.
typedef unsigned int control_flag_t;

// The control bits:
control_flag_t const nonewline_cf      		= 0x0001;
control_flag_t const noprefix_cf       		= 0x0002;
control_flag_t const nolabel_cf        		= 0x0004;
control_flag_t const blank_margin_cf       	= 0x0008;
control_flag_t const blank_label_cf       	= 0x0010;
control_flag_t const blank_marker_cf       	= 0x0020;
control_flag_t const cerr_cf           		= 0x0040;
control_flag_t const flush_cf          		= 0x0080;
control_flag_t const wait_cf           		= 0x0100;
control_flag_t const error_cf	   		= 0x0200;
control_flag_t const continued_cf_maskbit	= 0x0400;
// The bits of all special channels:
control_flag_t const fatal_maskbit     		= 0x0800;
control_flag_t const coredump_maskbit  		= 0x1000;
control_flag_t const continued_maskbit 		= 0x2000;
control_flag_t const finish_maskbit    		= 0x4000;

// Its own type for overloading (saves us an `if'):
enum continued_cf_nt {
  continued_cf
};

inline control_flag_t const cond_nonewline_cf(bool cond) { return cond ? nonewline_cf : 0; }
inline control_flag_t const cond_noprefix_cf(bool cond) { return cond ? noprefix_cf : 0; }
inline control_flag_t const cond_nolabel_cf(bool cond) { return cond ? nolabel_cf : 0; }
inline control_flag_t const cond_error_cf(bool err) { return err ? error_cf : 0; }

// The channels:
class channel_ct;
class fatal_channel_ct;
class continued_channel_ct;

//=============================================================================
//
// class channel_ct
//
// This object represents a debug channel.
//

class channel_ct {
private:
  // These friends only read data.
  friend class debug_ct;
  friend struct channel_set_st;

  mutable int off_cnt;
    // A counter of the nested calls to off().
    // The channel is turned off when the value of `off' is larger or equal then zero
    // and `on' when it has the value -1.

  char label[max_label_len];
    // A reference name for the represented debug channel
    // This label will be printed in front of each output written to
    // this debug channel.

public:
  //---------------------------------------------------------------------------
  // Constructor
  //

  explicit channel_ct(char const* lbl);
    // Constructor for an arbitrary new debug channel with label `lbl'.
    // A newly created channel is off by default.

  void initialize(char const* lbl) const { if (!*label) new (const_cast<channel_ct*>(this)) channel_ct(lbl); }
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.

public:
  //---------------------------------------------------------------------------
  // Manipulators
  //

  void off(void) const;
    // Turn this channel off.

  void on(void) const;
    // Cancel one call to `off()'. The channel is turned on when `on()' is
    // called as often as `off()' was called before.

public:
  //---------------------------------------------------------------------------
  // Accessors
  //

  bool is_on(void) const { return (off_cnt < 0); }
  char const* get_label(void) const { return label; }
};

//=============================================================================
//
// class fatal_channel_ct
//
// A debug channel with a special characteristic: It terminates the application.
//

class fatal_channel_ct {
private:
  // These friends only read data.
  friend class debug_ct;
  friend struct channel_set_st;

  char label[max_label_len];
    // A reference name for the represented debug channel
    // This label will be printed in front of each output written to
    // this debug channel.

  control_flag_t const maskbit;
    // The mask that contains the control bit.

public:
  //---------------------------------------------------------------------------
  // Constructor
  //

  explicit fatal_channel_ct(char const* lbl, control_flag_t cb);
    // Construct a special debug channel with label `lbl' and control bit `cb'.

  void initialize(char const* lbl, control_flag_t cb) const { if (!*label) new (const_cast<fatal_channel_ct*>(this)) fatal_channel_ct(lbl, cb); }
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.
};

//=============================================================================
//
// class continued_channel_ct
//
// A debug channel with a special characteristic: It uses the same label and
// flags as the previous Dout() call (no prefix is printed unless other
// debug output interrupted the start, indicated with `continued_cf').
//

class continued_channel_ct {
private:
  // This friend only reads data.
  friend class debug_ct;

  control_flag_t const maskbit;
    // The mask that contains the control bit.

public:
  //---------------------------------------------------------------------------
  // Constructor
  //

  explicit continued_channel_ct(control_flag_t cb) : maskbit(cb) { }
    // Construct a continued debug channel with extra control bit `cb'.

  void initialize(control_flag_t cb) const { if (maskbit == 0) new (const_cast<continued_channel_ct*>(this)) continued_channel_ct(cb); }
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.
};

//=============================================================================
//
// struct channel_set_data_st
//
// The attributes of channel_set_st and continued_channel_set_st
//

struct channel_set_data_st {
  //---------------------------------------------------------------------------
  // Attributes
  //

  char const* label;
    // The label of the most left channel that is turned on.

  control_flag_t mask;
    // The bit-wise OR mask of all control flags and special channels.

  bool on;
    // Set if at least one of the provided channels is turned on.

  class debug_ct* debug_object;
    // The owner of this object.
};

//=============================================================================
//
// struct continued_channel_set_st
//
// The debug output target; a combination of channels and control bits.
//

struct continued_channel_set_st : channel_set_data_st {
  // Warning: This struct may not have attributes of its own!

  //---------------------------------------------------------------------------
  // Operator
  //

  continued_channel_set_st& operator|(control_flag_t cf)
  {
    mask |= cf;
    return *this;
  }
};

//=============================================================================
//
// struct channel_set_st
//
// The debug output target; a combination of channels and control bits.
//

struct channel_set_st : channel_set_data_st {
  //---------------------------------------------------------------------------
  // Operators
  //

  channel_set_st& operator|(control_flag_t cf)
  {
    mask |= cf;
    return *this;
  }

  channel_set_st& operator|(channel_ct const& dc)
  {
    if (!on)
    {
      label = dc.label;
      on = (dc.off_cnt < 0);	// Private access to channel_ct::off_cnt, using < 0 instead of == -1 because thats faster.
    }
    return *this;
  }

  channel_set_st& operator|(fatal_channel_ct const& fdc)
  {
    mask |= fdc.maskbit;
    if (!on)
    {
      label = fdc.label;
      on = true;
    }
    return *this;
  }

  continued_channel_set_st& operator|(continued_cf_nt);
    // The returned set is a cast of this object.
};

//=============================================================================
//
// Internal classes - these interfaces are subject to change without
//                    notice: Do NOT use them outside libcw itself!
//

class laf_ct;

// My own stack implementation, one that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template<typename T>		// T must be a builtin type.
  struct debug_stack_tst {
  private:
    T st[64];
    T* p;
    T* end;
  public:
    void init(void) {
      p = &st[-1];
      end = &st[63];
    }
    void push(T ptr) {
      if (p == end)
	raise(3);	// This is really not normal, if you core here you probably did something wrong.
			// Doing a back trace in gdb should reveal an `infinite' debug output re-entrance loop.
			// This means that while printing debug output you call a function that makes
			// your program return to the same line, starting to print out that debug output
			// again. Try to break this loop some how.
      *++p = ptr;
    }
    void pop(void) { --p; }
    T top(void) { return *p; }
    size_t size(void) { return p - &st[-1]; }
  };

// string place holder (we can't use a string because that has a constructor).

struct buf_st {
  size_t len;
  char const* str;
  char const* orig_str;
  bool str_is_allocated;
  void init(char const* s, size_t l, bool first_time = false);
};

//=============================================================================
//
// class debug_ct
//
// Note: Debug output is printed already *before* this object is constructed,
// and is still printed when this object is already destructed.
// This is why initialization is done with method init() *before* construction
// and debug is turned off when this object is destructed.
// I hope that this is no problem because debug::libcw_do is a global object.
// It means however that this object can not contain any attributes that have
// a constructor of their own!

class debug_ct {
  //---------------------------------------------------------------------------
  // Attributes 
  //

public: // Direct access needed in macro LibcwDout().  Do not write to these.
  int _off;
    // Debug output is turned on when this variable is -1, otherwise it is off.

  laf_ct* current;
    // Current laf.

  std::ostream* current_oss;
    // The stringstream of the current laf.  This should *always* be equal to current->oss.
    // The reason for keeping this copy is to avoid including <sstream> in debug.h.

  union {
    channel_set_st           channel_set;
    continued_channel_set_st continued_channel_set;
  };
    // Temporary storage for the current (continued) channel set (while being assembled from operator| calls).
    // The reason for the union is that the type of this variable is converted from channel_set_st to
    // continued_channel_set_st by a reinterpret_cast when a continued_cf, dc::continued or dc::finish is
    // detected. This allows us to make compile-time decisions (using overloading).

protected:
  std::ostream* orig_os;
    // The original output ostream (as set with set_ostream() or set_fd()).

  std::ostream* os;
    // The current output ostream (may be a temporal stringstream).

  bool start_expected;
    // Set to true when start() is expected, otherwise we expect a call to finish().

  bool unfinished_expected;
    // Set to true when start() should cause a <unfinished>.

  debug_stack_tst<laf_ct*> laf_stack;
    // Store for nested debug calls.

  friend continued_channel_set_st& channel_set_st::operator|(continued_cf_nt);
    // Needs access to `initialized', `off_count' and `continued_stack'.

  int off_count;
    // Number of nested and switched off continued channels till first switched on continued channel.

  debug_stack_tst<int> continued_stack;
    // Stores the number of nested and switched off continued channels.

protected:
  //---------------------------------------------------------------------------
  // Attributes that determine how the prefix is printed:
  //

  struct buf_st margin;
    // This is printed before the label.

  struct buf_st marker;
    // This is printed after the label.

  unsigned short indent;
    // Position at which debug message is printed.
    // A value of 0 means directly behind the marker.

public:
  //---------------------------------------------------------------------------
  // Manipulators and accessors for the above "format" attributes:
  //

  void set_indent(unsigned short i) { indent = i; }
  void set_margin(std::string const& s);
  void set_marker(std::string const& s);

  unsigned short get_indent(void) const { return indent; }
  std::string get_margin(void) const;
  std::string get_marker(void) const;

  //---------------------------------------------------------------------------
  // Other accessors
  //

  std::ostream* get_ostream(void) const { return orig_os; }
  std::ostream& get_os(void) const { return *os; }

private:
  //---------------------------------------------------------------------------
  // Private attributes: 
  //

  std::ostream* saved_os;
    // The saved current output ostream while writing forced cerr.

  bool interactive;
    // Set true if the last or current debug output is to cerr

  bool initialized;
    // Set to true when this object is initialized (by a call to init()).

#ifdef DEBUGDEBUG
  long init_magic;
    // Used to check if the trick with `initialized' really works.
#endif

private:
  //---------------------------------------------------------------------------
  // Initialization function.
  //

  friend class channel_ct;
  friend class fatal_channel_ct;
  friend void initialize_globals(void);
  void init(void);
    // Initialize this object, needed because debug output can be written
    // from the constructors of (other) global objects, and from the malloc()
    // family when DEBUGMALLOC is defined.

public:
  //---------------------------------------------------------------------------
  // Constructors and destructors.
  //

  debug_ct(void) { init(); }
    // Constructor

  ~debug_ct();
    // Destructor.

public:
  //---------------------------------------------------------------------------
  // Manipulators:
  //

  void set_ostream(std::ostream* _os) { orig_os = os = _os; }
    // Assign a new ostream to this debug object (default is cerr).

  void off(void) { ++_off; }
    // Turn this debug object off.

#ifndef DEBUGDEBUG
  void on(void) { --_off; }
    // Cancel last call to off().
#else
  // Since with DEBUGDEBUG defined we start with _off is -1 instead of 0,
  // we need to ignore the call to on() the first time it is called.
private:
  bool first_time;
public:
  void on(void) { if (first_time) first_time = false; else --_off; }
#endif

  //===========================================================================
public: // Only public because these are accessed from LibcwDout().
	// You should not call them directly.

  void start(void);
  void finish(void);
  void fatal_finish(void) __attribute__ ((__noreturn__));

  //---------------------------------------------------------------------------
  // Operators that combine channels/control bits.
  //

  channel_set_st& operator|(channel_ct const& dc)
  {
    channel_set.mask = 0;
    channel_set.label = dc.label;
    channel_set.on = (dc.off_cnt < 0);	// Private access to channel_ct::off_cnt, using < 0 instead != -1 because thats faster.
    return channel_set;
  }

  channel_set_st& operator|(fatal_channel_ct const& fdc)
  {
    channel_set.mask = fdc.maskbit;
    channel_set.label = fdc.label;
    channel_set.on = true;
    return channel_set;
  }

  continued_channel_set_st& operator|(continued_channel_ct const& cdc);
};

namespace channels {
  namespace dc {
    extern channel_ct const debug;
    extern channel_ct const notice;
    extern channel_ct const system;
    extern channel_ct const warning;
#ifdef DEBUGMALLOC
    extern channel_ct const __libcwd_malloc;
#else
    extern channel_ct const malloc;
#endif
    extern fatal_channel_ct const fatal;
    extern fatal_channel_ct const core;
    extern continued_channel_ct const continued;
    extern continued_channel_ct const finish;
  } // namespace dc
} // namespace channels
extern debug_ct libcw_do;
typedef std::vector<debug_ct*> debug_objects_ct;
class debug_objects_singleton_ct {
  debug_objects_ct* _debug_objects;
public:
  void init(void);
  void uninit(void);
  debug_objects_ct& operator()(void) {
    if (!_debug_objects)
      init();
    return *_debug_objects;
  }
};
extern debug_objects_singleton_ct debug_objects;
typedef std::vector<channel_ct*> debug_channels_ct;
class debug_channels_singleton_ct {
  debug_channels_ct* _debug_channels;
  void init(void);
public:
  debug_channels_ct& operator()(void) {
    if (!_debug_channels)
      init();
    return *_debug_channels;
  }
};
extern debug_channels_singleton_ct debug_channels;

extern channel_ct const* find_channel(char const* label);
extern void list_channels_on(debug_ct const& debug_object);

  }	// namespace debug
}	// namespace libcw

#ifdef DEBUGDEBUG
#define DEBUGDEBUGLIBCWDOUTMARKER DEBUGDEBUG_CERR( "LibcwDout at " << __FILE__ << ':' << __LINE__ ); libcw::debug::debugdebugcheckpoint();
#else // !DEBUGDEBUG
#define DEBUGDEBUGLIBCWDOUTMARKER
#endif // !DEBUGDEBUG

#define LibcwDout( dc_namespace, debug_obj, cntrl, data )	\
  do								\
  {								\
    DEBUGDEBUGLIBCWDOUTMARKER					\
    if (debug_obj._off < 0)					\
    {								\
      using namespace ::libcw::debug;				\
      bool on;							\
      {								\
        using namespace dc_namespace;				\
	on = (debug_obj|cntrl).on;				\
      }								\
      if (on)							\
      {								\
	debug_obj.start();					\
	(*debug_obj.current_oss) << data;			\
	debug_obj.finish();					\
      }								\
    }								\
  } while(0)

#ifdef DEBUGDEBUG
#define DEBUGDEBUGLIBCWDOUTFATALMARKER DEBUGDEBUG_CERR( "LibcwDoutFatal at " << __FILE__ << ':' << __LINE__ ); libcw::debug::debugdebugcheckpoint();
#else // !DEBUGDEBUG
#define DEBUGDEBUGLIBCWDOUTFATALMARKER
#endif // !DEBUGDEBUG

#define LibcwDoutFatal( dc_namespace, debug_obj, cntrl, data )	\
  do							\
  {							\
    DEBUGDEBUGLIBCWDOUTFATALMARKER			\
    using namespace ::libcw::debug;			\
    {							\
      using namespace dc_namespace;			\
      debug_obj|cntrl;					\
    }							\
    debug_obj.start();					\
    (*debug_obj.current_oss) << data;			\
    debug_obj.fatal_finish();				\
  } while(0)

// For use in library header files
#define __Dout(cntrl, data) LibcwDout(::libcw::debug::channels, ::libcw::debug::libcw_do, cntrl, data)
#define __DoutFatal(cntrl, data) LibcwDoutFatal(::libcw::debug::channels, ::libcw::debug::libcw_do, cntrl, data)

// For use in applications
#define Dout(cntrl, data) LibcwDout(DEBUGCHANNELS, ::libcw::debug::libcw_do, cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(DEBUGCHANNELS, ::libcw::debug::libcw_do, cntrl, data)

#else // !CWDEBUG

// No debug output code
#define LibcwDout(a, b, c, d)
#define LibcwDoutFatal(a, b, c, d) do { ::std::cerr << d << ::std::endl; exit(-1); } while(1)
#define __Dout(a, b)
#define __DoutFatal(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)
#define Dout(a, b)
#define DoutFatal(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)

#endif // !CWDEBUG

#include <libcw/debugmalloc.h>
#if !defined(DEBUGMALLOC) && defined(DEBUGUSEBFD)
#include <libcw/bfd.h>
#endif

#ifdef CWDEBUG

// Make the inserter functions of std accessible in libcw::debug:
namespace libcw {
  namespace debug {
    using std::operator<<;
  }
}

#endif // CWDEBUG

#if defined(CWDEBUG) || defined(DEBUGMALLOC)

// Make the inserter functions of libcw::debug accessible in global namespace:
namespace libcw_debug_inserters {
  using libcw::debug::operator<<;
}
using namespace libcw_debug_inserters;

#endif // defined(CWDEBUG) || defined(DEBUGMALLOC)

#endif // LIBCW_DEBUG_H
