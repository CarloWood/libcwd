include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H3>Headers</H3>

<H1>&lt;H1&gt;Largest Header&lt;/H1&gt;</H1>
<H2>&lt;H2&gt;Here is some example code: &lt;code&gt;<code>libcw::debug::dc</code>&lt;/code&gt;&lt;/H2&gt;</H2>
<H3>&lt;H3&gt;Here is some example code: &lt;code&gt;<code>Dout(dc::notice, i &lt;&lt &quot;Hello&quot;)</code>&lt;/code&gt;&lt;/H3&gt;</H3>
<H4>&lt;H4&gt;Here is some example code: &lt;code&gt;<code>Dout(dc::notice, i &lt;&lt &quot;Hello&quot;)</code>&lt;/code&gt;&lt;/H4&gt;</H4>
<H5>&lt;H5&gt;Here is some example code: &lt;code&gt;<code>Dout(dc::notice, i &lt;&lt &quot;Hello&quot;)</code>&lt;/code&gt;&lt;/H5&gt;</H5>
<H6>&lt;H6&gt;Here is some example code: &lt;code&gt;<code>Dout(dc::notice, i &lt;&lt &quot;Hello&quot;)</code>&lt;/code&gt;&lt;/H6&gt;</H6>

<H3>Paragraphs</H3>

<p class="test-size2">Normal text with size 2. abcdefghijklmnopqrstuvwxyz 0123456789.</p>
<p class="test-size3">Normal text with size 3. abcdefghijklmnopqrstuvwxyz 0123456789.</p>
<p class="test-size4">Normal text with size 4. abcdefghijklmnopqrstuvwxyz 0123456789.</p>
<p class="test-size5">Normal text with size 5. abcdefghijklmnopqrstuvwxyz 0123456789.</p>
<p class="test-size6">Normal text with size 6. abcdefghijklmnopqrstuvwxyz 0123456789.</p>

<H3>Code</H3>

<P>&lt;p&gt;Normal text.&nbsp;You can use <code>libcwd</code>
for instance in space shuttles or nuclear power plants,
to write debug output to <code>ostream</code>
devices.&lt;/p&gt;</P>

<P>&lt;code&gt;<code>func(char const*)</code>&lt;/code&gt;</P>

<P>&lt;pre&gt;</P>
<PRE>// This line is not indented.
  // This one is indented two spaces.
  int abcdefghijklmnopqrstuvwxyz = 0123456789;</PRE>
<P>&lt;/pre&gt;</P>

<P>Here is program output:</P>

<PRE class="output">
Output of a program
should always be shown
inside a &lt;PRE&gt;
tag.
</PRE>

<H3>Examples</H3>

<P>This is an <SPAN class="example">example</SPAN> that is inline.</P>

<P class="example">This is an example by itself.</P>

<PRE class="example">
NOTICE: This is example output.
NOTICE: This is the second line.
</PRE>

__PAGEEND
__PAGEFOOTER
__HTMLFOOTER

