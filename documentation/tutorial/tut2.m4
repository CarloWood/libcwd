include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 2: Creating your own Debug Channels</H2>

<P>You can easily create your own debug channels.&nbsp;
In the example below we create a debug channel <CODE>dc::ghost</CODE>
that will use the string "<SPAN class="output">GHOST</SPAN>" as label.</P>

<P class="download">[<A HREF="channel.cc">download</A>]</P>

<P>Create a file <CODE>"sys.h"</CODE> that is part of your application and put in it:</P>
<PRE>
#ifdef HAVE_CONFIG_H		// This is just an example of what you could do
#include "config.h"		//   when using autoconf for your project.
#endif
#ifdef CWDEBUG		// This is needed so that others can compile
#include &lt;libcw/sysd.h&gt	//   your application without having libcwd installed.
#endif
</PRE>
<P>Create a file <CODE>"debug.h"</CODE> that is part of your application and put in it:</P>
<PRE>
#ifndef DEBUG_H
#define DEBUG_H

#ifndef CWDEBUG

#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, x)
#define AllocTag(p, x)
#define Debug(x)
#define Dout(a, b)
#define DoutFatal(a, b) LibcwDoutFatal(::std, , a, b)
#define ForAllDebugChannels(STATEMENT)
#define ForAllDebugObjects(STATEMENT)
#define LibcwDebug(dc_namespace, x)
#define LibcwDout(a, b, c, d)
#define LibcwDoutFatal(a, b, c, d) do { ::std::cerr << d << ::std::endl; ::std::exit(254); } while(1)
#define NEW(x) new x

#else // CWDEBUG

#ifndef DEBUGCHANNELS
// This must be defined before <libcw/debug.h> is included and must be the
// name of the namespace containing your `dc' namespace (see below).
// You can use any namespace(s) you like, except existing namespaces
// (like ::, ::std and ::libcw).
#define DEBUGCHANNELS ::myproject::debug::channels
#endif
#include <libcw/debug.h>

namespace myproject {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace ::libcw::debug::channels::dc;

	// Add the declaration of new debug channels here
	// and their definition in a custom debug.cc file.
	extern ::libcw::debug::channel_ct custom;

      } // namespace dc
    } // namespace DEBUGCHANNELS
  }
}

#endif // CWDEBUG
#endif // DEBUG_H
</PRE>
<P>Finally write the program:</P>
<PRE>
#include "sys.h"
#include "debug.h"

// Include this to make life easier.
using namespace libcw::debug;

// Actual definition of `ghost'
namespace debug_channels {
  namespace dc {
    channel_ct <SPAN class="highlight">ghost</SPAN>("GHOST");
  }
}

int main(void)
{
  Debug( dc::ghost.on() );			// Remember: don't forget to turn
  Debug( libcw_do.on() );			//   the debug Channel and Object on!

  for (int i = 0; i &lt; 4; ++i)
    Dout( <SPAN class="highlight">dc::ghost</SPAN>, "i = " &lt;&lt; i );		// We can write more than just
					      // "Hello World" to the ostream :)
  return 0;
}
</PRE>

<P>This program outputs:</P>

<PRE class="output">
GHOST   : i = 0
GHOST   : i = 1
GHOST   : i = 2
GHOST   : i = 3
</PRE>

<P>Note that when writing a <EM>library</EM> you are highly advised to follow the namespace guideline
as set forth in the <A HREF="../html/group__chapter__custom__debug__h.html#libraries">Reference Manual</A>.</P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#label">What is the maximum length of a label?</A></LI>
<LI><A HREF="faq.html#prefix">Why do I have to use the <CODE>dc::</CODE> prefix?</A></LI>
<LI><A HREF="faq.html#labelwidth">Why does it print spaces between the label and the colon?&nbsp; How is the field width of the label determined?</A></LI>
</UL></DIV>

__PAGEEND
<P class="line"><IMG width=870 height=26 src="../images/lines/ghost.png"></P>
<DIV class="buttons">
<A HREF="tut1.html"><IMG width=64 height=32 src="../images/buttons/lr_prev.png" border=0></A>
<A HREF="index.html"><IMG width=64 height=32 src="../images/buttons/lr_index.png" border="0"></A>
<A HREF="tut3.html"><IMG width=64 height=32 src="../images/buttons/lr_next.png" border="0"></A>
</DIV>
__PAGEFOOTER
__HTMLFOOTER
