include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 2: Creating your own Debug Channels</H2>

<P>You can easily create your own debug channels.&nbsp;
In the example below we create a debug channel <CODE>dc::ghost</CODE>
that will use the string "<SPAN class="output">GHOST</SPAN>" as label.</P>

<P>Create a file <CODE>"debug.h"</CODE> that is part of your application and put in it:</P>
<PRE>
#ifndef DEBUG_H
#define DEBUG_H

#ifndef CWDEBUG

#include <iostream>
#include <cstdlib>

#define Debug(...)
#define Dout(cntrl, ...)
#define DoutFatal(cntrl, ...) LibcwDoutFatal(, , cntrl, __VA_ARGS__)
#define ForAllDebugChannels(...)
#define ForAllDebugObjects(...)
#define LibcwDebug(dc_namespace, ...)
#define LibcwDout(dc_namespace, d, cntrl, ...)
#define LibcwDoutFatal(dc_namespace, d, cntrl, ...) \
  do                                                \
  {                                                 \
    ::std::cerr << __VA_ARGS__ << ::std::endl;      \
    ::std::exit(EXIT_FAILURE);                      \
  } while (1)
#define LibcwdForAllDebugChannels(dc_namespace, ...)
#define LibcwdForAllDebugObjects(dc_namespace, ...)
#define CWDEBUG_LOCATION 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGT 0

#else // CWDEBUG

// This must be defined before &lt;libcwd/debug.h&gt; is included and must
// be the name of the namespace containing your `dc' namespace (see below).
// You can use any namespace(s) you like, except existing namespaces
// (like std or libcwd). It may not start with two colons because it is used
// for both `namespace LIBCWD_DEBUG_CHANNELS {` as well as
// `using namespace ::LIBCWD_DEBUG_CHANNELS`: the first namespace is implied
// to be a global namespace already.
#define LIBCWD_DEBUG_CHANNELS myproject::debug::channels
#include &lt;libcwd/debug.h&gt;

NAMESPACE_DEBUG_CHANNELS_START

// Add the declaration of new debug channels here
// and their definition in a custom debug.cpp file.
extern Channel custom;

NAMESPACE_DEBUG_CHANNELS_END

#endif // CWDEBUG
#endif // DEBUG_H
</PRE>
<P>Finally write the program:</P>
<P class="download">[<A HREF="channel.cpp">download</A>]</P>
<PRE>
#include "debug.h"

NAMESPACE_DEBUG_CHANNELS_START          // In this case this is example::channels
Channel <SPAN class="highlight">ghost</SPAN>("GHOST");                 // Create our own Debug Channel.
NAMESPACE_DEBUG_CHANNELS_END

int main()
{
  Debug(NAMESPACE_DEBUG::init());       // Mandatory call to notify the library that
                                        // main() was reached, check that the correct
                                        // headers are being used and to read the rcfile.

  // Lets not forget to turn the debug Channel and Object on!
  Debug(if (!dc::ghost.is_on()) dc::ghost.on()); // Might already be on due to rcfile.
  Debug(libcw_do.on());

  for (int i = 0; i &lt; 4; ++i)
    Dout(<SPAN class="highlight">dc::ghost</SPAN>, "i = " &lt;&lt; i);       // We can write more than just
                                        // "Hello World" to the ostream :)
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
as set forth in the <A HREF="../reference-manual/group__chapter__custom__debug__h.html#libraries">Reference Manual</A>.</P>

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
