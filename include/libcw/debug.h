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
#ifdef __GNUG__
#pragma interface
#endif
#define LIBCW_DEBUG_H

#include <libcw/debugging_defs.h>
#include <iosfwd>

RCSTAG_H(debug, "$Id$")

#  ifdef DEBUG
#   ifdef DEBUGNONAMESPACE
#     define USING_NAMESPACE_LIBCW_DEBUG
#     define NAMESPACE_LIBCW_DEBUG ::std
#     define LIBCW
#   else
#     define USING_NAMESPACE_LIBCW_DEBUG using namespace ::libcw::debug;
#     define NAMESPACE_LIBCW_DEBUG ::libcw::debug
#     define LIBCW ::libcw
#    endif
#    define UNUSED_UNLESS_DEBUG(x) x
#    include <assert.h>
#    define ASSERT(x) assert(x);
#    define Debug(x) do { USING_NAMESPACE_LIBCW_DEBUG/**/(x); } while(0)
#    define ForAllDebugObjects(STATEMENT) \
       for( NAMESPACE_LIBCW_DEBUG::debug_objects_ct::iterator __libcw_i(NAMESPACE_LIBCW_DEBUG::debug_objects->begin()); __libcw_i != NAMESPACE_LIBCW_DEBUG::debug_objects->end(); ++__libcw_i) \
       { \
         USING_NAMESPACE_LIBCW_DEBUG \
         debug_ct& debugObject(*(*__libcw_i)); \
	 STATEMENT; \
       }
#    define ForAllDebugChannels(STATEMENT) \
       for( NAMESPACE_LIBCW_DEBUG::debug_channels_ct::iterator __libcw_i(NAMESPACE_LIBCW_DEBUG::debug_channels->begin()); __libcw_i != NAMESPACE_LIBCW_DEBUG::debug_channels->end(); ++__libcw_i) \
       { \
         USING_NAMESPACE_LIBCW_DEBUG \
         channel_ct& debugChannel(*(*__libcw_i)); \
	 STATEMENT; \
       }
#  else
#    ifdef __cplusplus
#      define UNUSED_UNLESS_DEBUG(x) /**/
#    else
#      define UNUSED_UNLESS_DEBUG(x) unused__##x
#    endif
#    define ASSERT(x)
#    define Debug(x)
#    define ForAllDebugObjects(STATEMENT)
#    define ForAllDebugChannels(STATEMENT)
#    define LIBCW ::libcw
#  endif

#ifdef DEBUG
#include <strstream>
#include <vector>
#include <string>
#ifdef DEBUGDEBUG
#include <libcw/debugdebugcheckpoint.h>
#endif

#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif

    //=============================================================================
    // Definitions

    // The maximum number of characters that are allowed in a debug channel label.
    unsigned short const max_label_len = 16;

    // Define the type that is used for control flags and control flag mask.
    typedef unsigned int control_flag_t;

    struct continued_cf_st {
      unsigned int maskbit;
      continued_cf_st(control_flag_t mb) : maskbit(mb) {}
    };

    // The control bits:
    control_flag_t const noprefix_cf       	= 0x01;
    control_flag_t const nolabel_cf        	= 0x02;
    control_flag_t const cerr_cf           	= 0x04;
    control_flag_t const flush_cf          	= 0x08;
    control_flag_t const wait_cf           	= 0x10;
    control_flag_t const error_cf	   	= 0x20;
    control_flag_t const nonewline_cf      	= 0x40;
    control_flag_t const continued_cf_maskbit	= 0x80;
    // The bits of all special channels:
    control_flag_t const fatal_maskbit     	= 0x100;
    control_flag_t const coredump_maskbit  	= 0x200;
    control_flag_t const continued_maskbit 	= 0x400;
    control_flag_t const finish_maskbit    	= 0x800;

    // Its own type for overloading (saves us an `if'):
    continued_cf_st const continued_cf(continued_cf_maskbit);

    inline control_flag_t const cond_noprefix_cf(bool err) { return err ? noprefix_cf : 0; }
    inline control_flag_t const cond_nolabel_cf(bool err) { return err ? nolabel_cf : 0; }
    inline control_flag_t const cond_error_cf(bool err) { return err ? error_cf : 0; }
    inline control_flag_t const cond_nonewline_cf(bool err) { return err ? nonewline_cf : 0; }

    // The channels:
    class channel_ct;
    class fatal_channel_ct;
    class continued_channel_ct;

    //=============================================================================
    //
    // class channel_ct
    //
    // This object represents a debug channel.
    // Debug channels should be declared within the namespace `::libcw::debug'.
    //
    // Example:
    //
    //  namespace libcw {
    //    namespace debug {
    //      channel_ct const my_channel("MY_LABEL");
    //    };
    //  };
    //
    //  Makes it possible to use:
    //
    //  Dout( my_channel, "foobar" );
    //
    //  Which will then output :
    //
    //  MY_LABEL: foobar
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

      channel_ct(char const* lbl);
	// Constructor for an arbitrary new debug channel with label `lbl'.
	// A newly created channel is off by default.

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

      fatal_channel_ct(char const* lbl, control_flag_t cb);
        // Construct a special debug channel with label `lbl' and control bit `cb'.
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

      continued_channel_ct(control_flag_t cb) : maskbit(cb) {}
        // Construct a continued debug channel with extra control bit `cb'.
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
	  on = (dc.off_cnt < 0);	// Private access to channel_ct::off_cnt, using < 0 instead of != -1 because thats faster.
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

      continued_channel_set_st& operator|(continued_cf_st cf);
        // The returned set is a cast of this object.
    };

    //=============================================================================
    //
    // Internal classes - these interfaces are subject to change without
    //                    notice: Do NOT use them outside libcw itself!
    //

#ifdef DEBUGMALLOC
    class no_alloc_checking_ostrstream : public ostrstream {
    private:
      strstreambuf *my_sb;
    public:
      no_alloc_checking_ostrstream(void);
      ~no_alloc_checking_ostrstream();
      strstreambuf* rdbuf() { return my_sb; }
    };    
#endif

    class laf_ct {
    public:
#ifndef DEBUGMALLOC
      ostrstream oss;
        // The temporary output buffer.
#else
      no_alloc_checking_ostrstream oss;
#endif
      int prefix_end;
        // Number of characters in oss that make up the prefix.

      size_t flushed;
        // Number of character in the buffer that are already flushed.

      control_flag_t mask;
        // The previous control bits.

      char const* label;
        // The previous label.

      ostream* saved_os;
        // The previous original ostream.

      int err;
        // The current errno.

    public:
      laf_ct(control_flag_t m, char const* l, ostream* os, int e) : flushed(0), mask(m), label(l), saved_os(os), err(e) {}
    };

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
			// This means that you while printing debug output you call a function that makes
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

    public: // Direct access needed in macro LibcwDout(). Do not write to these.
      int _off;
	// True when the debug output is turned off.

      laf_ct* current;
        // Current laf.

      union {
	channel_set_st           channel_set;
	continued_channel_set_st continued_channel_set;
      };
	// Temporary storage for the current (continued) channel set (while being assembled from operator| calls).
	// The reason for the union is that the type of this variable is converted from channel_set_st to
	// continued_channel_set_st by a reinterpret_cast when a continued_cf, continued_dc or finish_dc is
	// detected. This allows us to make compile-time decisions (using overloading).

    protected:
      ostream* orig_os;
        // The original output ostream (as set with set_ostream() or set_fd()).

      ostream* os;
        // The current output ostream (may be a temporal ostrstream).

      bool start_expected;
        // Set to true when start() is expected, otherwise we expect a call to finish().

      bool unfinished_expected;
        // Set to true when start() should cause a <unfinished>.

      debug_stack_tst<laf_ct*> laf_stack;
        // Store for nested debug calls.

      friend continued_channel_set_st& channel_set_st::operator|(continued_cf_st cf);
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
      void set_margin(string const& s);
      void set_marker(string const& s);

      unsigned short get_indent(void) const { return indent; }
      string get_margin(void) const;
      string get_marker(void) const;

      //---------------------------------------------------------------------------
      // Other accessors
      //

      ostream* get_ostream(void) const { return orig_os; }
      ostream& get_os(void) const { return *os; }

    private:
      //---------------------------------------------------------------------------
      // Private attributes: 
      //

      ostream* saved_os;
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

      void set_ostream(ostream* _os) { orig_os = os = _os; }
        // Assign a new ostream to this debug object (default is cerr).

      void off(void) { ++_off; }
        // Turn this debug object off.

#ifndef DEBUGDEBUG
      void on(void) { --_off; }
        // Cancel last call to off().
#else
      // Since with DEBUGDEBUG defined we start with _off is 0 instead of 1,
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

    extern channel_ct const debug_dc;
    extern channel_ct const notice_dc;
    extern channel_ct const system_dc;
    extern channel_ct const warning_dc;
    extern channel_ct const malloc_dc;
    extern fatal_channel_ct const fatal_dc;
    extern fatal_channel_ct const core_dc;
    extern continued_channel_ct const continued_dc;
    extern continued_channel_ct const finish_dc;
    extern debug_ct libcw_do;
    typedef vector<debug_ct*> debug_objects_ct;
    extern debug_objects_ct* debug_objects;
    typedef vector<channel_ct*> debug_channels_ct;
    extern debug_channels_ct* debug_channels;

    extern channel_ct const* find(char const* label);
    extern void list_channels_on(debug_ct const& debug_object);

#ifndef DEBUGNONAMESPACE
  };	// namespace debug
};	// namespace libcw
#endif

#ifdef DEBUGDEBUG
#define DEBUGDEBUGLIBCWDOUTMARKER  cerr << "DEBUGDEBUG: LibcwDout at " << __FILE__ << ':' << __LINE__ << '\n'; debugdebugcheckpoint();
#else
#define DEBUGDEBUGLIBCWDOUTMARKER
#endif

#define LibcwDout( dc_namespace, debug_obj, cntrl, data )	\
  do								\
  { DEBUGDEBUGLIBCWDOUTMARKER					\
    if (!debug_obj._off)					\
    {								\
      bool on;							\
      {								\
        using namespace dc_namespace;				\
	on = (debug_obj|cntrl).on;				\
      }								\
      if (on)							\
      {								\
	debug_obj.start();					\
	debug_obj.current->oss << data;				\
	debug_obj.finish();					\
      }								\
    }								\
  } while(0)

//
// This macro is undocumented because you shouldn't use it :)
// Its only here because this is the only Right Place(tm)
// to put it and I needed it for bfd.cc
//
#define LibcwDout_vform( dc_namespace, debug_obj, cntrl, format, vl )	\
  do								\
  { DEBUGDEBUGLIBCWDOUTMARKER					\
    if (!debug_obj._off)					\
    {								\
      bool on;							\
      {								\
        using namespace dc_namespace;				\
	on = (debug_obj|cntrl).on;				\
      }								\
      if (on)							\
      {								\
	debug_obj.start();					\
	debug_obj.current->oss.vform(format, vl);		\
	debug_obj.finish();					\
      }								\
    }								\
  } while(0)

#ifdef DEBUGDEBUG
#define DEBUGDEBUGLIBCWDOUTFATALMARKER  cerr << "DEBUGDEBUG: LibcwDoutFatal at " << __FILE__ << ':' << __LINE__ << '\n'; debugdebugcheckpoint();
#else
#define DEBUGDEBUGLIBCWDOUTFATALMARKER
#endif

#define LibcwDoutFatal( dc_namespace, debug_obj, cntrl, data )	\
  do							\
  { DEBUGDEBUGLIBCWDOUTFATALMARKER			\
    {							\
      using namespace dc_namespace;			\
      debug_obj|cntrl;					\
    }							\
    debug_obj.start();					\
    debug_obj.current->oss << data;			\
    debug_obj.fatal_finish();				\
  } while(0)

#define __Dout(cntrl, data) LibcwDout(NAMESPACE_LIBCW_DEBUG, NAMESPACE_LIBCW_DEBUG::libcw_do, cntrl, data)
#define __Dout_vform(cntrl, format, vl) LibcwDout_vform(NAMESPACE_LIBCW_DEBUG, NAMESPACE_LIBCW_DEBUG::libcw_do, cntrl, format, vl)
#define __DoutFatal(cntrl, data) LibcwDoutFatal(NAMESPACE_LIBCW_DEBUG, NAMESPACE_LIBCW_DEBUG::libcw_do, cntrl, data)

#else // !DEBUG

#define LibcwDout(a, b, c)
#define LibcwDoutFatal(a, b, c) do { cerr << c << endl; exit(-1); } while(1)
#define __Dout(a, b)
#define __Dout_vform(a, b, c)
#define __DoutFatal(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)

#endif // DEBUG

#define Dout(cntrl, data) __Dout(cntrl, data)
#define Dout_vform(cntrl, format, vl) __Dout_vform(cntrl, format, vl)
#define DoutFatal(cntrl, data) __DoutFatal(cntrl, data)

#include <libcw/debugmalloc.h>

#endif // LIBCW_DEBUG_H
