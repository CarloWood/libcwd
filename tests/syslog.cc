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

//
// This code is intended to serve as an example of how to write
// an object that catches debug output of libcwd and writes it to
// syslog(3).
//
// Compile as:
//
// g++ -g -DCWDEBUG -c syslog.cc
// g++ syslog.o -lcwd -o syslog
//
// On less divine operating systems as linux, you might need to
// link using: g++ syslog.o -lcwd -lbfd -liberty -o syslog
//

#include "sys.h"
#include <syslog.h>
#include <libcw/buf2str.h>
// #include <locale> Not supported by early versions
#include "syslog_debug.h"

using namespace std;

// New debug object which will write to syslog(3)
libcw::debug::debug_ct syslog_do;
// and a new debug channel to test this program
namespace syslog_example {
  namespace debug {
    namespace channels {
      namespace dc {
	libcw::debug::channel_ct const debug_syslog("SYSLOG");
      }
    }
  }
}

class syslog_streambuf_ct : public streambuf {
  // Because we do not use an ANSI/ISO C++ conforming libstdc++, we need to add a few typedefs here.
  typedef streambuf basic_streambuf;
  typedef ios ios_base;
  typedef char char_type;
  typedef streampos pos_type;
  typedef streamoff off_type;
  typedef int int_type;
  struct traits { static int_type const eof(void) { return static_cast<int_type>(-1); } };
private:
  char M_buf[256];	// Tiny buffer, doesn't need to contain more than one line.
  int M_priority;
public:
  syslog_streambuf_ct(int priority) : M_priority(priority) { setbuf(M_buf, sizeof(M_buf)); }
protected:
  // The ANSI/ISO C++ virtual streambuf interface:
// Not supported by early versions of libstdc++: virtual void imbue(locale const&);
  virtual basic_streambuf* setbuf(char_type* s, streamsize n);
  virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which = ios_base::in | ios_base::out);
  virtual pos_type seekpos(pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out);
  virtual int sync(void);
  virtual int showmanyc(void);
  virtual streamsize xsgetn(char_type* s, streamsize n);
  virtual int/*_type*/ underflow(); // The old library returns an `int'
  virtual int/*_type*/ uflow(); // The old library returns an int
  virtual int_type pbackfail(int_type c = traits::eof());
  virtual streamsize xsputn(const char_type* s, streamsize n);
  virtual int_type overflow(int_type c = traits::eof());
};

class syslog_stream_ct : public ostream {
private:
  syslog_streambuf_ct M_buf;
public:
  syslog_stream_ct(char const* ident, int option, int  facility, int priority);
  ~syslog_stream_ct();
};

syslog_stream_ct::syslog_stream_ct(char const* ident, int option, int  facility, int priority) : ostream(&M_buf), M_buf(priority)
{
  // Note: because of this, syslog_stream_ct should actually be a singleton, but I am lazy now.
  openlog(const_cast<char*>(ident), option, facility);
}

syslog_stream_ct::~syslog_stream_ct()
{
  closelog();
}

int main(int, char* argv[])
{
  //----------------------------------------------------------------------
  // The following calls will be done in almost every program using libcwd

  Debug( check_configuration() );

#ifdef DEBUGMALLOC
  // Don't show allocations that are allocated before main()
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

  // Select channels (note that where 'on' is used, 'off' can be specified
  // and vica versa).
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );

  // Write debug output to cout (the default is cerr)
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on.  Turning it off can be done recursively.
  // It starts with 'off' depth 1.
  Debug( libcw_do.on() );

  // List all debug channels (not very usefull unless you allow to turn
  // channels on and off from the commandline; this is supported in libcw).
  Debug( list_channels_on(libcw_do) );

  // End of common block
  //----------------------------------------------------------------------

  syslog_stream_ct syslog_stream(argv[0], LOG_CONS|LOG_PID, LOG_DAEMON, LOG_CRIT);

  // Write debug output to syslog_stream (the default is cerr)
  Debug( syslog_do.set_ostream(&syslog_stream) );

  // Turn our debug object on!
  Debug( syslog_do.on() );

  Dout(dc::notice, "Program start, next we will write something to the syslog");

  // Write 20 times "Hello syslog!" to syslog:
  for(int i = 0; i < 20; ++i)
    SyslogDout(dc::notice, "Hello syslog! (" << i << ")");

  return 0;
}

//-----------------------------------------------------------------------------
// Now the real fun begins

// From ANSI C++ draft (1997)
// 27.5.2.4  basic_streambuf virtual functions		[lib.streambuf.virtuals]

// 27.5.2.4.1  Locales					[lib.streambuf.virt.locales]

// Effects:
//   Change any translations based on locale.
// Notes:
//   Allows  the derived class to be informed of changes in locale at the
//   time they occur.  Between  invocations  of  this  function  a  class
//   derived  from  streambuf can safely cache results of calls to locale
//   functions and to members of facets so obtained.
// Default behavior:
//   Does nothing.
// Not supported by early versions:
// void syslog_streambuf_ct::imbue(locale const&) { }

// 27.5.2.4.2  Buffer management and positioning		[lib.streambuf.virt.buffer]

// Effects:
//   Performs  an  operation  that  is  defined separately for each class
//   derived from basic_streambuf in this  clause  (_lib.stringbuf.virtu-
//   als_, _lib.filebuf.virtuals_).
// Default behavior:
//   If  gptr() is non-null and gptr()!=egptr() then do nothing.  Returns
//   this.
syslog_streambuf_ct::basic_streambuf* syslog_streambuf_ct::setbuf(syslog_streambuf_ct::char_type* s, streamsize n)
{
  Dout(dc::debug_syslog, "Calling: setbuf(" << (void*)s << ", " << n << ')');
  memset(s, 0, n);	// Just for fun (not needed)
  setp(s, s + n);	// Define Put Area
  setg(s, s, s + n);	// Define Get Area (same as put area)
  return this;
}

// Effects:
//   Alters the stream positions within one or  more  of  the  controlled
//   sequences in a way that is defined separately for each class derived
//   from  basic_streambuf  in  this  clause   (_lib.stringbuf.virtuals_,
//   _lib.filebuf.virtuals_).
// Default behavior:
//   Returns  an  object  of class pos_type that stores an invalid stream
//   position (_lib.iostreams.definitions_).
syslog_streambuf_ct::pos_type syslog_streambuf_ct::seekoff(syslog_streambuf_ct::off_type off, ios_base::seekdir way, ios_base::openmode which = ios_base::in | ios_base::out)
{
  Dout(dc::warning|dc::debug_syslog, "Calling: seekoff(" << off << ", " << way << ", " << which << ')');
  return seekoff(off, way, which);
}

// Effects:
//   Alters the stream positions within one or  more  of  the  controlled
//   sequences in a way that is defined separately for each class derived
//   from basic_streambuf in  this  clause  (_lib.stringbuf_,  _lib.file-
//   buf_).
// Default behavior:
//   Returns  an  object  of class pos_type that stores an invalid stream
//   position.
syslog_streambuf_ct::pos_type syslog_streambuf_ct::seekpos(syslog_streambuf_ct::pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out)
{
  Dout(dc::warning|dc::debug_syslog, "Calling: seekpos(" << sp << ", " << which << ')');
  return streambuf::seekpos(sp, which);
}

// Effects:
//   Synchronizes the controlled sequences with the arrays.  That is,  if
//   pbase()  is  non-null  the characters between pbase() and pptr() are
//   written to the controlled sequence.  The pointers may then be  reset
//   as appropriate.
// Returns:
//   -1  on  failure.   What  constitutes  failure  is determined by each
//  derived class (_lib.filebuf.virtuals_).
//  Default behavior:
//  Returns zero.
int syslog_streambuf_ct::sync(void)
{
  Dout(dc::debug_syslog, "Calling: sync()");
  Dout(dc::debug_syslog, "eback() = " << (void*)eback());
  Dout(dc::debug_syslog, "gptr() = " << (void*)gptr());
  Dout(dc::debug_syslog, "egptr() = " << (void*)egptr());
  Dout(dc::debug_syslog, "Get area: \"" << buf2str(eback(), egptr() - eback()) << '"');
  Dout(dc::debug_syslog, "Get area data: \"" << buf2str(gptr(), pptr() - gptr()) << '"'); // Would be gptr() till egptr() when put area != get area
  Dout(dc::debug_syslog, "pbase() = " << (void*)pbase());
  Dout(dc::debug_syslog, "pptr() = " << (void*)pptr());
  Dout(dc::debug_syslog, "epptr() = " << (void*)epptr());
  Dout(dc::debug_syslog, "Put area: \"" << buf2str(pbase(), epptr() - pbase()) << '"');
  Dout(dc::debug_syslog, "Put area data: \"" << buf2str(gptr(), pptr() - gptr()) << '"'); // Would be pbase() till pptr() when put area != get area
  // Search for a newline, starting at the back: we assume there is only one newline and in most cases it will be at the end.
  for (char* p = pptr() - 1; p >= gptr(); --p)
    if (*p == '\n')
    {
      *p = 0;
      syslog(M_priority, "%s", gptr());
      if (p == pptr() - 1)		// Was the newline the last in the buffer?
	setbuf(M_buf, sizeof(M_buf));	// Reset buffer pointers (buffer is now empty), to avoid buffer overrun
      break;
    }
  return 0;
}

// 27.5.2.4.3  Get area					[lib.streambuf.virt.get]

// Returns:
//   an  estimate  of the number of characters available in the sequence,
//   or -1.  If it returns a positive value,  then  successive  calls  to
//   underflow() will not return traits::eof() until at least that number
//   of characters have been supplied.  If showmanyc() returns  -1,  then
//   calls to underflow() or uflow() will fail.
// Default behavior:
//   Returns zero.
// Notes:
//   Uses traits::eof().
int syslog_streambuf_ct::showmanyc(void)
{
  Dout(dc::warning|dc::debug_syslog, "Calling: showmanyc()");
  return -1;	// Write only
}

// Effects:
//   Assigns up to n characters to successive elements of the array whose
//   first element is designated by s.  The characters assigned are  read
//   from  the  input  sequence  as  if  by  repeated  calls to sbumpc().
//   Assigning stops when either n characters have  been  assigned  or  a
//   call to sbumpc() would return traits::eof().
// Returns:
//   The number of characters assigned.
// Notes:
//   Uses traits::eof().
streamsize syslog_streambuf_ct::xsgetn(syslog_streambuf_ct::char_type* s, streamsize n)
{
  Dout(dc::warning|dc::debug_syslog, "Calling: xsgetn(" << (void*)s << ", " << n << ')');
  return 0;	// Write only, and we don't use xsgetn ourselfs.
}

// Notes:
//   The  public  members  of  basic_streambuf call this virtual function
//   only if gptr() is null or gptr() >= egptr()
// Returns:
//   traits::to_int_type(c), where c is the first character of the  pend-
//   ing  sequence,  without  moving the input sequence position past it.
//   If  the  pending  sequence  is  null  then  the   function   returns
//   traits::eof() to indicate failure.
//
// 1 The pending sequence of characters is defined as the concatenation of:
//
// a)If gptr() is non- NULL, then the egptr() - gptr() characters  start-
//   ing at gptr(), otherwise the empty sequence.
//
// b)Some  sequence  (possibly  empty)  of characters read from the input
//   sequence.
//
// 2 The result character is
//
// a)If the pending sequence is non-empty, the  first  character  of  the
//   sequence.
//
// b)If  the pending sequence empty then the next character that would be
//   read from the input sequence.
//
// 3 The backup sequence is defined as the concatenation of:
//
// a)If eback() is null then empty,
//
// b)Otherwise the gptr() - eback() characters beginning at eback().
// Effects:
//   The function sets up the gptr() and egptr() satisfying one of:
//
// a)If the pending  sequence  is  non-empty,  egptr()  is  non-null  and
//   egptr() - gptr() characters starting at gptr() are the characters in
//   the pending sequence
//
// b)If the pending sequence is empty, either gptr() is  null  or  gptr()
//   and egptr() are set to the same non-NULL pointer.
//
// 4 If  eback()  and  gptr()  are  non-null  then the function is not con-
// strained as to their contents, but the ``usual backup  condition''  is
// that either:
//
// a)If  the  backup  sequence contains at least gptr() - eback() charac-
//   ters, then the gptr() - eback() characters starting at eback() agree
//   with the last gptr() - eback() characters of the backup sequence.
//
// b)Or  the  n  characters  starting at gptr() - n agree with the backup
//   sequence (where n is the length of the backup sequence)
// Default behavior:
//   Returns traits::eof().
int/*_type*/ syslog_streambuf_ct::underflow() // The old library returns an `int'
{
  Dout(dc::warning|dc::debug_syslog, "Calling: underflow()");
  // We should never get here: we are write only.
  return streambuf::underflow();
}

// Requires:
//   The constraints are the same as for  underflow(),  except  that  the
//   result  character  is  transferred  from the pending sequence to the
//   backup sequence, and the pending sequence may not  be  empty  before
//   the transfer.
// Default behavior:
//   Calls  underflow().   If  underflow() returns traits::eof(), returns
//   traits::eof().      Otherwise,     returns     the     value      of
//   traits::to_int_type(*gptr())  and  increment  the  value of the next
//   pointer for the input sequence.
// Returns:
//   traits::eof() to indicate failure.
int/*_type*/ syslog_streambuf_ct::uflow() // The old library returns an int
{
  Dout(dc::warning|dc::debug_syslog, "Calling: uflow()");
  // We should never get here: we are write only.
  return streambuf::uflow();
}

// 27.5.2.4.4  Putback					[lib.streambuf.virt.pback]

// Notes:
//   The public functions of basic_streambuf call this  virtual  function
//   only    when    gptr()    is    null,    gptr()   ==   eback(),   or
//   traits::eq(*gptr(),traits::to_char_type(c))  returns  false.   Other
//   calls shall also satisfy that constraint.
//   The pending sequence is defined as for underflow(), with the modifi-
//   cations that
//
// --If traits::eq_int_type(c,traits::eof()) returns true, then the input
//   sequence  is  backed up one character before the pending sequence is
//   determined.
//
// --If traits::eq_int_type(c,traits::eof())  return  false,  then  c  is
//   prepended.   Whether  the input sequence is backed up or modified in
//   any other way is unspecified.
// Postcondition:
//   On return, the constraints of gptr(), eback(), and  pptr()  are  the
//   same as for underflow().
// Returns:
//   traits::eof()  to  indicate  failure.  Failure may occur because the
//   input sequence could not be backed up, or if for some  other  reason
//   the  pointers  could  not  be  set  consistent with the constraints.
//   pbackfail() is called only when put back has really failed.
//   Returns some value other than traits::eof() to indicate success.
// Default behavior:
//   Returns traits::eof().
syslog_streambuf_ct::int_type syslog_streambuf_ct::pbackfail(int_type c = syslog_streambuf_ct::traits::eof())
{
  Dout(dc::warning|dc::debug_syslog, "Calling: pbackfail(" << c << ')');
  // We should never get here, we never put anything back on the stream (since we never read from it either).
  return streambuf::pbackfail(c);
}

// 27.5.2.4.5  Put area					[lib.streambuf.virt.put]

// Effects:
//   Writes up to n characters to the output sequence as if  by  repeated
//   calls to sputc(c).  The characters written are obtained from succes-
//   sive elements of the array whose first element is designated  by  s.
//   Writing  stops  when either n characters have been written or a call
//   to sputc(c) would return traits::eof().
// Returns:
//   The number of characters written.
streamsize syslog_streambuf_ct::xsputn(syslog_streambuf_ct::char_type const* s, streamsize n)
{
  Dout(dc::debug_syslog, "Calling: xsputn(\"" << buf2str(s, n) << "\", " << n << ')');
  // Write this to our buffer (no error checking yet)
  strncpy(pptr(), s, n);
  pbump(n);
  return n;
}

// Effects:
//   Consumes some initial subsequence of the characters of  the  pending
//   sequence.  The pending sequence is defined as the concatenation of
//
// a)if  pbase()  is  NULL  then  the  empty sequence otherwise, pptr() -
//   pbase() characters beginning at pbase().
//
// b)if traits::eq_int_type(c,traits::eof()) returns true, then the empty
//   sequence otherwise, the sequence consisting of c.
// Notes:
//   The  member functions sputc() and sputn() call this function in case
//   that no room can be found in the put buffer enough to accomodate the
//   argument character sequence.
// Requires:
//   Every  overriding definition of this virtual function shall obey the
//   following constraints:
//
// 1)The effect  of  consuming  a  character  on  the  associated  output
//   sequence is specified.
//
// 2)Let r be the number of characters in the pending sequence  not  con-
//   sumed.   If  r  is  non-zero  then pbase() and pptr() must be set so
//   that: pptr() - pbase() == r and the r characters starting at pbase()
//   are  the  associated output stream.  In case r  is zero (all charac-
//   ters of the pending sequence have been consumed) then either pbase()
//   is  set to NULL, or pbase() and pptr() are both set to the same non-
//   NULL value.
//
// 3)The function may fail if either  appending  some  character  to  the
//   associated  output  stream  fails  or  if  it is unable to establish
//   pbase() and pptr() according to the above rules.
// Returns:
//   traits::eof() or throws an exception if the function fails.
//   Otherwise, returns some value other than traits::eof()  to  indicate
//   success.
// Default behavior:
//   Returns traits::eof().
syslog_streambuf_ct::int_type syslog_streambuf_ct::overflow(syslog_streambuf_ct::int_type c = syslog_streambuf_ct::traits::eof())
{
  Dout(dc::debug_syslog, "Calling: overflow(" << c << ')');
  return 0;
}
