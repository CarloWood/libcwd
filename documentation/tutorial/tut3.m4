include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 3: Setting the <CODE>ostream</CODE></H2>

<P>You can write the debug output to any given ostream.&nbsp;
The following example opens a file <SPAN class="filename">log</SPAN> and
uses it to write its debug output to.</P>

<P class="download">[<A HREF="log_file.cc">download</A>]</P>

<P>Compile as: <CODE>g++ -DCWDEBUG log_file.cc -lcwd -o log_file</CODE></P>
<PRE>
#define _GNU_SOURCE
#include &lt;libcw/sysd.h&gt;
#include &lt;fstream&gt;
#include &lt;libcw/debug.h&gt;

int main(void)
{
  Debug( dc::notice.on() );
  Debug( libcw_do.on() );

#ifdef CWDEBUG
  ofstream file;
  file.open("log");
#endif

  // Set the ostream related with libcw_do to `file':&nbsp;&nbsp;
  <SPAN class="highlight">Debug( libcw_do.set_ostream(&amp;file) );</SPAN>

  Dout( dc::notice, "Hippopotamus are heavy" );

  return 0;
}
</PRE>

<P>Debug code like the definition of the debug file <CODE>file</CODE>,
should be put between <CODE>#ifdef CWDEBUG ... #endif</CODE> as usual.&nbsp;
This isn't needed for <CODE>Debug()</CODE> or <CODE>Dout()</CODE>
because these macros are automatically replaced with white space
when <CODE>CWDEBUG</CODE> is not defined.</P>

__PAGEEND
<P class="line"><IMG width=870 height=19 src="../images/lines/hippo.png"></P>
<DIV class="buttons">
<A HREF="tut2.html"><IMG width=64 height=32 src="../images/buttons/lr_prev.png" border="0"></A>
<A HREF="index.html"><IMG width=64 height=32 src="../images/buttons/lr_index.png" border="0"></A>
<A HREF="tut4.html"><IMG width=64 height=32 src="../images/buttons/lr_next.png" border="0"></A>
</DIV>
__PAGEFOOTER
__HTMLFOOTER
