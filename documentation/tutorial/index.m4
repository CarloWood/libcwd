include(definitions.m4)dnl
define(__TOC_START, [[
</DIV>
<TABLE border=0 cellpadding=0 cellspacing=10>

<TR>
<TD WIDTH=15 valign=center>
  <IMG width=34 height=34 src="../images/toc.png" alt="Main index" align=top border=0>
</TD>
<TD valign=center><DIV class="toc-header1">
  <A class="toc1" href="../html/index.html" target="_top">
    Back to main index
  </A></DIV>
</TD>
</TR>
]])dnl
define(__TOC_END, [[
</TABLE>
<DIV class="normal">
]])dnl
define(__PAGE, [[<!-- -Page $1---------------------------------------------------- -->

<TR>
<TD WIDTH=15 valign=center>
  <IMG width=34 height=34 src="../images/toc.png" align=top border=0>
</TD>
<TD valign=center><DIV class="toc-header1">
  <A class="toc1" href="$3">
    $2
  </A></DIV>
</TD>
</TR>]])dnl
define(__PAGE_START, [[
<TR>
<TD WIDTH=15></TD>
<TD>
  <TABLE border=0 cellpadding=0 cellspacing=0>]])dnl
define(__PAGE_END, [[
  </TABLE>
</TD>
</TR>]])dnl
define(__PARAGRAPH, [[
<TR>
<TD WIDTH=50><DIV class="toc-number$1">
    $2
</DIV></TD>
<TD><DIV class="toc-header$1">
  <A class="toc" HREF="$4">
    $3
  </A></DIV>
</TD>
</TR>]])dnl
dnl#
dnl# Start of document
dnl#
__HTMLHEADER
__PAGEHEADER
__PAGESTART
<P>Welcome to the tutorial of libcwd, an Object Oriented debugging support library for C++ developers.</P>

<H1>Table of Contents</H1>
__TOC_START
__PAGE(1, [[Introduction]], intro.html)
__PAGE(2, [[Tutorial 1 : Hello World]], tut1.html)
__PAGE(3, [[Tutorial 2 : Creating your own Debug Channels]], tut2.html)
__PAGE(4, [[Tutorial 3 : Setting the <CODE>ostream</CODE>]], tut3.html)
__PAGE(5, [[Tutorial 4 : Management of <CODE>CWDEBUG</CODE>]], tut4.html)
__PAGE(6, [[Tutorial 5 : Advanced examples]], tut5.html)
__PAGE_START
__PARAGRAPH(2f, 5.1, [[Running over all Debug Channels]], [[tut5.html#Running]])
__PARAGRAPH(2, 5.2, [[Debug Channels and name spaces]], [[tut5.html#Debug]])
__PARAGRAPH(2, 5.3, [[Combining channels]], [[tut5.html#Combining]])
__PARAGRAPH(2, 5.4, [[Formatting debug output]], [[tut5.html#Formatting]])
__PARAGRAPH(3, 5.4.1, [[Control flags]], [[tut5.html#Control]])
__PARAGRAPH(3, 5.4.2, [[Methods of the debug object]], [[tut5.html#Methods]])
__PAGE_END
__PAGE(7, [[Tutorial 6 : The debugging of dynamic memory allocations]], tut6.html)
__PAGE(8, [[Tutorial 7 : Advanced examples]], tut7.html)
__PAGE_START
__PARAGRAPH(2f, 7.1, [[Removing allocations from the Allocated memory Overview]], [[tut7.html#Removing]])
__PARAGRAPH(2, 7.2, [[Retrieving information about memory allocations]], [[tut7.html#Retrieving]])
__PARAGRAPH(2, 7.3, [[Memory leak detection]], [[tut7.html#Memory]])
__PAGE_END
__TOC_END
__PAGEEND
__PAGEFOOTER
__HTMLFOOTER
