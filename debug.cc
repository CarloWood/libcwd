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

#include <libcw/sys.h>
#include <libcw/debug_config.h>
#ifdef CWDEBUG
#include <errno.h>
#include <signal.h>		// Needed for raise()
#include <iostream>
#include <sys/time.h>     	// Needed for setrlimit()
#include <sys/resource.h>	// Needed for setrlimit()
#include <algorithm>
#include <new>
#include <libcw/debug.h>
#include <libcw/strerrno.h>
#include <libcw/no_alloc_checking_stringstream.h>
#ifdef DEBUGUSEBFD
#include <libcw/bfd.h>		// Needed for location_ct
#endif
#endif // CWDEBUG

RCSTAG_CC("$Id$")

using namespace std;

#ifdef CWDEBUG

#ifdef DEBUGMALLOC
#define debug_alloc_checking_on() set_alloc_checking_off()
#define debug_alloc_checking_off() set_alloc_checking_on()
#else
#define debug_alloc_checking_on()
#define debug_alloc_checking_off()
#endif

namespace libcw {
  namespace debug {

    class buffer_ct : public no_alloc_checking_stringstream {
    private:
      pos_type position;
    public:
      void writeto(ostream* os)
      {
	int curlen = rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out) - rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
	for (char c = rdbuf()->sgetc(); --curlen >= 0; c = rdbuf()->snextc())
	{
#ifdef DEBUGMALLOC
	  // Writing to the final ostream (ie cerr) must be non-internal!
	  bool saved_internal = _internal_::internal;
	  _internal_::internal = false;
	  ++_internal_::library_call;
	  ++libcw_do._off;
#endif
	  os->put(c);
#ifdef DEBUGMALLOC
	  --libcw_do._off;
	  --_internal_::library_call;
	  _internal_::internal = saved_internal;
#endif
        }
      }
      void store_position(void) {
	position = rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out);
      }
      void restore_position(void) {
	rdbuf()->pubseekoff(position, ios_base::beg, ios_base::out);
	rdbuf()->pubseekoff(0, ios_base::beg, ios_base::in);
      }
    };

    // Configuration signature
    unsigned long config_signature_lib = config_signature_header;

    // Put this here to decrease the code size of `check_configuration'
    void conf_check_failed(void)
    {
      DoutFatal(dc::fatal, "check_configuration: This version of libcwd was compiled with a different configuration than is currently used in libcw/debug_config.h!");
    }

    debug_ct libcw_do;			// The Debug Object that is used by default by Dout(), the only debug object used
					// by libcw itself.

    namespace {
      unsigned short int max_len = 8;	// The length of the longest label. Is adjusted automatically
    					// if a custom channel has a longer label.
    }

    namespace channels {
      namespace dc {
	channel_ct const debug("DEBUG");
	channel_ct const notice("NOTICE");
	channel_ct const warning("WARNING");
	channel_ct const system("SYSTEM");
	channel_ct const malloc("MALLOC");

	continued_channel_ct const continued(continued_maskbit);
	continued_channel_ct const finish(finish_maskbit);

	fatal_channel_ct const fatal("FATAL", fatal_maskbit);
	fatal_channel_ct const core("COREDUMP", coredump_maskbit);
      }
    }

    void initialize_globals(void)
    {
      libcw_do.init();
    }

    debug_channels_singleton_ct debug_channels;	// List with all channel_ct objects.
    debug_objects_singleton_ct debug_objects;		// List with all debug devices.

    void debug_channels_singleton_ct::init(void)
    {
      if (!_debug_channels)
      {
        set_alloc_checking_off();
        _debug_channels = new debug_channels_ct;
        set_alloc_checking_on();
      }
    }

    void debug_objects_singleton_ct::init(void)
    {
      if (!_debug_objects)
      {
        DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#ifdef DEBUGMALLOC
	// It is possible that malloc is not initialized yet.
	init_debugmalloc();
#endif
        set_alloc_checking_off();
        _debug_objects = new debug_objects_ct;
        set_alloc_checking_on();
      }
    }

    void debug_objects_singleton_ct::uninit(void)
    {
      if (_debug_objects)
      {
	set_alloc_checking_off();
	delete _debug_objects;
	set_alloc_checking_on();
	_debug_objects = NULL;
      }
    }

#ifdef DEBUGDEBUG
    static long debug_object_init_magic = 0;

    static void init_debug_object_init_magic(void)
    {
      struct timeval rn;
      gettimeofday(&rn, NULL);
      debug_object_init_magic = rn.tv_usec;
      if (!debug_object_init_magic)
        debug_object_init_magic = 1;
      DEBUGDEBUG_CERR( "Set debug_object_init_magic to " << debug_object_init_magic );
    }
#endif

    class laf_ct {
    public:
      buffer_ct oss;
	// The temporary output buffer.

      control_flag_t mask;
	// The previous control bits.

      char const* label;
	// The previous label.

      ostream* saved_os;
	// The previous original ostream.

      int err;
	// The current errno.

    public:
      laf_ct(control_flag_t m, char const* l, ostream* os, int e) : mask(m), label(l), saved_os(os), err(e) { }
    };

    static inline void write_whitespace_to(ostream& os, unsigned int size)
    {
      for (unsigned int i = size; i > 0; --i)
        os.put(' ');
    }

    void debug_ct::start(void)
    {
      // It's possible we get here before this debug object is initialized: The order of calling global is undefined :(.
      // Hopefully `initialized' is set to 0 anyway for all uninitialized global objects(?).
      if (!initialized)			
        init();
#ifdef DEBUGDEBUG
      else if (init_magic != debug_object_init_magic || !debug_object_init_magic)
      {
        DEBUGDEBUG_CERR( "Ack, fatal error in libcwd: `initialized' true while `init_magic' not set!  "
	        "Please mail the author of libcwd." );
	raise(3);	// Core dump
      }
#endif

      debug_alloc_checking_on();

      // Skip `start()' for a `continued' debug output.
      // Generating the "prefix: <continued>" is already taken care of while generating the "<unfinished>" (see next `if' block).
      if ((channel_set.mask & (continued_maskbit|finish_maskbit)))
      {
        current->mask = channel_set.mask;	// New bits might have been added
	current->err = errno;			// Always keep the last errno as set at the start of LibcwDout()
#ifdef DEBUGMALLOC
	ASSERT( _internal_::internal == 1 );
#endif
        return;
      }

      ++_off;
      DEBUGDEBUG_CERR( "Entering debug_ct::start(), _off became " << _off );

      // Is this an interrupting debug output (in the middle of a continued debug output)?
      if ((current->mask & continued_cf_maskbit) && unfinished_expected)
      {
        // Write out what is in the buffer till now.
        current->oss.writeto(os);
	// Append <unfinished> to it.
	os->write("<unfinished>\n", 13);	// Continued debug output should end on a space by itself,
	// Truncate the buffer to its prefix and append "<continued>" to it already.
	current->oss.restore_position();
	current->oss << "<continued> ";		// therefore we repeat the space here.
      }

      // By putting this here instead of in finish(), all nested debug output will go to cerr too.
      if ((channel_set.mask & cerr_cf))
      {
        saved_os = os;
	os = &cerr;
      }

      // Is this a nested debug output (the first of a series in the middle of another debug output)?
      if (!start_expected)
      {
	// Put current stringstream on the stack.
	laf_stack.push(current);

	// Indent nested debug output with 4 extra spaces.
	indent += 4;
      }

      // Without a new nested Dout() call, we expect to see a finish() call: The finish belonging to *this* Dout() call.
      start_expected = false;

      // If this is a `continued' debug output, then we want to print "<unfinished>" if next we see a start().
      unfinished_expected = true;

      // Create a new laf.
      //set_alloc_checking_off();
      DEBUGDEBUG_CERR( "creating new laf_ct" );
      current = new laf_ct(channel_set.mask, channel_set.label, saved_os, errno);
      current_oss = &current->oss;
      DEBUGDEBUG_CERR( "laf_ct created" );
      //set_alloc_checking_on();

      // Print prefix if requested.
      // Handle most common case first: no special flags set
      if (!(channel_set.mask & (noprefix_cf|nolabel_cf|blank_margin_cf|blank_label_cf|blank_marker_cf)))
      {
	current->oss.write(margin.str, margin.len);
	current->oss.write(channel_set.label, max_len);
	current->oss.write(marker.str, marker.len);
	write_whitespace_to(current->oss, indent);
      }
      else if (!(channel_set.mask & noprefix_cf))
      {
	if ((channel_set.mask & blank_margin_cf))
	  write_whitespace_to(current->oss, margin.len);
	else
	  current->oss.write(margin.str, margin.len);
#ifndef DEBUGDEBUG
	if (!(channel_set.mask & nolabel_cf))
#endif
	{
	  if ((channel_set.mask & blank_label_cf))
	    write_whitespace_to(current->oss, max_len);
	  else
	    current->oss.write(channel_set.label, max_len);
	  if ((channel_set.mask & blank_marker_cf))
	    write_whitespace_to(current->oss, marker.len);
	  else
	    current->oss.write(marker.str, marker.len);
	  write_whitespace_to(current->oss, indent);
	}
      }

      // If this is continued debug output, then it makes sense to remember the prefix length,
      // just in case we need indeed to output <continued> data (it wouldn't hurt to do this
      // always, but it is rarely a continued debug output and the mask test is faster than
      // calling store_position().
      if ((channel_set.mask & continued_cf_maskbit))
        current->oss.store_position();

      --_off;
      DEBUGDEBUG_CERR( "Leaving debug_ct::start(), _off became " << _off );

#ifdef DEBUGMALLOC
      // While writing debug output (which is directly after returning from this function)
      // alloc checking needs to be on.  FIXME
      ASSERT( _internal_::internal == 1 );
#endif
    }

    static char dummy_laf[sizeof(laf_ct)] __attribute__((__aligned__));

    void debug_ct::finish(void)
    {
      // Skip `finish()' for a `continued' debug output.
      if ((current->mask & continued_cf_maskbit) && !(current->mask & finish_maskbit))
      {
        if ((current->mask & continued_maskbit))
	  unfinished_expected = true;

        // If the `flush_cf' control flag is set, flush the ostream at every `finish()' though.
	if ((current->mask & flush_cf))
	{
	  // Write buffer to ostream.
	  current->oss.writeto(os);
	  // Flush ostream. Note that in the case of nested debug output this `os' can be an stringstream,
	  // in that case, no actual flushing is done until the debug output to the real ostream has
	  // finished.
	  *os << flush;
	}
	debug_alloc_checking_off();
        return;
      }

      ++_off;
      DEBUGDEBUG_CERR( "Entering debug_ct::finish(), _off became " << _off );

      // Write buffer to ostream.
      current->oss.writeto(os);
      channel_set.mask = current->mask;
      channel_set.label = current->label;
      saved_os = current->saved_os;
      //set_alloc_checking_off();
      DEBUGDEBUG_CERR( "Deleting `current'" );
      if (current == reinterpret_cast<laf_ct*>(dummy_laf))
      {
	*os << '\n';
	char const* channame = (channel_set.mask & finish_maskbit) ? "finish" : "continued";
#ifdef DEBUGUSEBFD
        DoutFatal(dc::core, "Using `dc::" << channame << "' in " <<
	    debug::location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset) <<
	    " without (first using) a matching `continue_cf'.");
#else
        DoutFatal(dc::core, "Using `dc::" << channame <<
	    "' without (first using) a matching `continue_cf'.");
#endif
      }
      delete current;
      DEBUGDEBUG_CERR( "Done deleting `current'" );
      //set_alloc_checking_on();

      // Handle control flags, if any:
      if (channel_set.mask == 0)
	*os << '\n';
      else
      {
	if ((channel_set.mask & error_cf))
	  *os << ": " << strerrno(current->err) << " (" << strerror(current->err) << ')';
	if ((channel_set.mask & coredump_maskbit))
	{
	  static bool debug_internal2 = false;
	  if (!debug_internal2)
	  {
	    debug_internal2 = true;
	    *os << endl;
	  }
	  raise(3);		// Core dump
	}
	if ((channel_set.mask & fatal_maskbit))
	{
	  static bool debug_internal2 = false;
	  if (!debug_internal2)
	  {
	    debug_internal2 = true;
	    *os << endl;
	  }
	  debug_alloc_checking_off();
	  exit(254);
	}
	if ((channel_set.mask & wait_cf))
	{
	  *os << "\n(type return)";
	  if (interactive)
	  {
	    *os << flush;
	    while(cin.get() != '\n');
	  }
	}
	if (!(channel_set.mask & nonewline_cf))
	  *os << '\n';
	if ((channel_set.mask & flush_cf))
	  *os << flush;
	if ((channel_set.mask & cerr_cf))
	  os = saved_os;
      }

      if (start_expected)
      {
        // Ok, we're done with the last buffer.
	indent -= 4;
        laf_stack.pop();
      }

      // Restore previous buffer as being the current one.
      if (laf_stack.size())
      {
        current = laf_stack.top();
	current_oss = &current->oss;
	if ((channel_set.mask & flush_cf))
	  current->mask |= flush_cf;	// Propagate flush to real ostream.
      }

      start_expected = true;
      unfinished_expected = false;

      --_off;
      DEBUGDEBUG_CERR( "Leaving debug_ct::finish(), _off became " << _off );

      debug_alloc_checking_off();
    }

    void debug_ct::fatal_finish(void)
    {
      finish();
      DoutFatal( dc::core, "Don't use `DoutFatal' together with `continue_cf', use `Dout' instead.  (This message can also occur when using DoutFatal correctly but from the constructor of a global object)." );
    }

    void debug_ct::init(void)
    {
      if (initialized)
        return;

      _off = 0;						// Turn off all debugging until initialization is completed.
      DEBUGDEBUG_CERR( "In debug_ct::init(void), _off set to 0" );

      set_alloc_checking_off();				// debug_objects is internal.
      if (find(debug_objects().begin(), debug_objects().end(), this) == debug_objects().end()) // Not added before?
	debug_objects().push_back(this);
      set_alloc_checking_on();

      // Initialize this debug object:
      set_ostream(&cerr);				// Write to cerr by default.
      interactive = true;				// and thus we're interactive.
      start_expected = true;				// Of course, we start with expecting the beginning of a debug output.
      continued_channel_set.debug_object = this;	// The owner of this continued_channel_set.
      // Fatal channels need to be marked fatal, otherwise we get into an endless loop
      // when they are used before they are created.
      channels::dc::core.initialize("COREDUMP", coredump_maskbit);
      channels::dc::fatal.initialize("FATAL", fatal_maskbit);
      // Initialize other debug channels that might be used before we reach main().
      channels::dc::debug.initialize("DEBUG");
      channels::dc::malloc.initialize("MALLOC");
      channels::dc::continued.initialize(continued_maskbit);
      channels::dc::finish.initialize(finish_maskbit);
      channels::dc::stabs.initialize("STABS");
      // What the heck, initialize all other debug channels too
      channels::dc::warning.initialize("WARNING");
      channels::dc::notice.initialize("NOTICE");
      channels::dc::system.initialize("SYSTEM");
      // `current' needs to be non-zero (saving us a check in start()) and
      // current.mask needs to be 0 to avoid a crash in start():
      debug_alloc_checking_on();
      current = new (dummy_laf) laf_ct(0, channels::dc::debug.label, NULL, 0);	// Leaks 24 bytes of memory
      current_oss = &current->oss;
      debug_alloc_checking_off();
      laf_stack.init();
      continued_stack.init();
      margin.init("", 0, true);				// Note: These first time strings need to be static/const.
      marker.init(": ", 2, true);

#if 0 // We assumed it was zeroed right? :)
      indent = 0;					// Don't do indenting.
      off_count = 0;					// No unfinished channels yet, let alone switched off ones.
      unfinished_expected = false;			// We didn't print anything yet, so we certainly don't expect to
      							// interrupt a previous debug output.
#endif

#ifdef DEBUGDEBUG
      _off = -1;		// Print as much debug output as possible right away.
      first_time = true;	// Needed to ignore the first time we call on().
#else
      _off = 0;			// Don't print debug output till the REAL initialization of the debug system has been performed
      				// (ie, the _application_ start (don't confuse that with the constructor - which does nothing)).
#endif
      DEBUGDEBUG_CERR( "After debug_ct::init(void), _off set to " << _off );

#ifdef DEBUGDEBUG
      if (!debug_object_init_magic)
	init_debug_object_init_magic();
      init_magic = debug_object_init_magic;
      DEBUGDEBUG_CERR( "Set init_magic to " << init_magic );
      DEBUGDEBUG_CERR( "Setting initialized to true" );
#endif
      initialized = true;

      // We want to do the following only once per application.
      static bool very_first_time = true;
      if (very_first_time)
      {
        very_first_time = false;
	// Unlimit core size.
#ifdef RLIMIT_CORE
	struct rlimit corelim;
	if (getrlimit(RLIMIT_CORE, &corelim))
	  DoutFatal(dc::fatal|error_cf, "getrlimit(RLIMIT_CORE, &corelim)");
	corelim.rlim_cur = corelim.rlim_max;
	if (corelim.rlim_max != RLIM_INFINITY)
	  cerr << "WARNING : core size is limited (hard limit: " << (corelim.rlim_max / 1024) << " kb).  Core dumps might be truncated!\n";
	if (setrlimit(RLIMIT_CORE, &corelim))
	    DoutFatal(dc::fatal|error_cf, "unlimit core size failed");
#else
	Dout(dc::warning, "Please unlimit core size manually");
#endif
      }
    }

    debug_ct::~debug_ct()
    {
      // Sanity checks:
      if (continued_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_ct with a non-empty continued_stack" );
      if (laf_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_ct with a non-empty laf_stack" );

      ++_off;		// Turn all debug output premanently off, otherwise we might re-initialize
                        // this object again when we try to write debug output to it!
      DEBUGDEBUG_CERR( "debug_ct destructed: _off became " << _off );
      initialized = false;
#ifdef DEBUGDEBUG
      init_magic = 0;
#endif
      marker.init("", 0);	// Free allocated memory
      margin.init("", 0);
      set_alloc_checking_off();	// debug_objects is internal.
      debug_objects().erase(find(debug_objects().begin(), debug_objects().end(), this));
      if (debug_objects().empty())
        debug_objects.uninit();
      set_alloc_checking_on();
    }

    void debug_ct::set_margin(string const& s) {
      ++_off;
      margin.init(s.data(), s.size());
      --_off;
    }

    void debug_ct::set_marker(string const& s) {
      ++_off;
      marker.init(s.data(), s.size());
      --_off;
    }

    string debug_ct::get_margin(void) const {
      return string(margin.str, margin.len);
    }

    string debug_ct::get_marker(void) const
    {
      return string(marker.str, marker.len);
    }

    channel_ct const* find_channel(char const* label)
    {
      channel_ct const* tmp = NULL;
      for(debug_channels_ct::iterator i(debug_channels().begin()); i != debug_channels().end(); ++i)
      {
        if (!strncasecmp(label, (*i)->get_label(), strlen(label)))
          tmp = (*i);
      }
      return tmp;
    }

    void list_channels_on(debug_ct const& debug_object)
    {
      if (debug_object._off < 0)
	for(debug_channels_ct::iterator i(debug_channels().begin()); i != debug_channels().end(); ++i)
	{
	  char const* txt = (*i)->is_on() ? ": Enabled" : ": Disabled";
	  debug_object.get_os().write((*i)->get_label(), max_len);
	  debug_object.get_os() << txt << '\n';
	}
    }

    channel_ct::channel_ct(char const* lbl)
    {
      // This is pretty much identical to fatal_channel_ct::fatal_channel_ct().

      if (*label)
        return;		// Already initialized.

      DEBUGDEBUG_CERR( "Entering `channel_ct::channel_ct(\"" << lbl << "\")'" );

      off_cnt = 0;

      // Of course, dc::debug is off - so this won't do anything unless DEBUGDEBUG is #defined.
      Dout( dc::debug, "Initializing channel_ct(\"" << lbl << "\")" );

      size_t lbl_len = strlen(lbl);

      if (lbl_len > max_label_len)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << lbl << "\") > " << max_label_len );

      if (lbl_len > max_len)
	max_len = lbl_len;

      strncpy(label, lbl, lbl_len);
      memset(label + lbl_len, ' ', max_label_len - lbl_len);

      // We store debug channels in some organized order, so that the
      // order in which they appear in the ForAllDebugChannels is not
      // dependent on the order in which these global objects are
      // initialized.
      debug_channels_ct::iterator i(debug_channels().begin());
      for(; i != debug_channels().end(); ++i)
        if (strncmp((*i)->label, label, max_label_len) > 0)
	  break;
      debug_channels().insert(i, this);

      DEBUGDEBUG_CERR( "Leaving `channel_ct::channel_ct(\"" << lbl << "\")" );
    }

    fatal_channel_ct::fatal_channel_ct(char const* lbl, control_flag_t cb) : maskbit(cb)
    {
       // This is pretty much identical to channel_ct::channel_ct().

      if (*label)
        return;		// Already initialized.

      DEBUGDEBUG_CERR( "Entering `fatal_channel_ct::fatal_channel_ct(\"" << lbl << "\")'" );

      // Of course, dc::debug is off - so this won't do anything unless DEBUGDEBUG is #defined.
      Dout( dc::debug, "Initializing fatal_channel_ct(\"" << lbl << "\")" );

      size_t lbl_len = strlen(lbl);

      if (lbl_len > max_label_len)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << lbl << "\") > " << max_label_len );

      if (lbl_len > max_len)
	max_len = lbl_len;

      strncpy(label, lbl, lbl_len);
      memset(label + lbl_len, ' ', max_label_len - lbl_len);

      DEBUGDEBUG_CERR( "Leaving `fatal_channel_ct::fatal_channel_ct(\"" << lbl << "\")" );
    }

    void channel_ct::off(void) const
    {
      ++off_cnt;
    }

    void channel_ct::on(void) const
    {
      if (off_cnt == -1)
	DoutFatal( dc::core, "Calling channel_ct::on() more often then channel_ct::off()" );
      --off_cnt;
    }

    //----------------------------------------------------------------------------------------
    // Handle continued channel flags and update `off_count' and `continued_stack'.
    //

    continued_channel_set_st& channel_set_st::operator|(continued_cf_nt)
    {
#ifdef DEBUGDEBUG
      DEBUGDEBUG_CERR( "continued_cf detected" );
      if (!debug_object || !debug_object->initialized)
        DEBUGDEBUG_CERR( "Don't use DoutFatal together with continued_cf, use Dout instead." );
#endif
      mask |= continued_cf_maskbit;
      if (!on)
      {
	++(debug_object->off_count);
	DEBUGDEBUG_CERR( "Channel is switched off. Increased off_count to " << debug_object->off_count );
      }
      else
      {
        debug_object->continued_stack.push(debug_object->off_count);
        DEBUGDEBUG_CERR( "Channel is switched on. Pushed off_count (" << debug_object->off_count << ") to stack (size now " <<
	    debug_object->continued_stack.size() << ") and set off_count to 0" );
        debug_object->off_count = 0;
      }
      return *(reinterpret_cast<continued_channel_set_st*>(this));
    }

    continued_channel_set_st& debug_ct::operator|(continued_channel_ct const& cdc)
    {
#ifdef DEBUGDEBUG
      if ((cdc.maskbit & continued_maskbit))
	DEBUGDEBUG_CERR( "dc::continued detected" );
      else
        DEBUGDEBUG_CERR( "dc::finish detected" );
#endif

      if ((continued_channel_set.on = !off_count))
      {
        DEBUGDEBUG_CERR( "Channel is switched on (off_count is 0)" );
	current->mask |= cdc.maskbit;                                 // We continue with the current channel
	continued_channel_set.mask = current->mask;
	continued_channel_set.label = current->label;
	if (cdc.maskbit == finish_maskbit)
	{
	  off_count = continued_stack.top();
	  continued_stack.pop();
	  DEBUGDEBUG_CERR( "Restoring off_count to " << off_count << ". Stack size now " << continued_stack.size() );
	}
      }
      else
      {
        DEBUGDEBUG_CERR( "Channel is switched off (off_count is " << off_count << ')' );
	if (cdc.maskbit == finish_maskbit)
	{
	  DEBUGDEBUG_CERR( "` decrementing off_count with 1" );
	  --off_count;
        }
      }
      return continued_channel_set;
    }

    channel_set_st& debug_ct::operator|(fatal_channel_ct const&)
    {
      DoutFatal(dc::fatal, location_ct(__builtin_return_address(0)) << " : Don't use Dout together with dc::core or dc::fatal!  Use DoutFatal instead.");
    }

    channel_set_st& debug_ct::operator&(channel_ct const&)
    {
      DoutFatal(dc::fatal, location_ct(__builtin_return_address(0)) << " : Use dc::core or dc::fatal together with DoutFatal.");
    }

    void buf_st::init(char const* s, size_t l, bool first_time)
    {
      if (first_time)
      {
        str = orig_str = s;
	len = l;
        str_is_allocated = false;
	return;
      }
      set_alloc_checking_off();
      if (str_is_allocated)
	delete [] const_cast<char*>(str);
      if ((len = l) > 0)
      {
        char* p = new char[l];
	memcpy(p, s, l);
	str = p;
	str_is_allocated = true;
      }
      else
      {
        str = orig_str;
	str_is_allocated = false;
	len = strlen(str);
      }
      set_alloc_checking_on();
    }

  }	// namespace debug
}	// namespace libcw

#endif // CWDEBUG
