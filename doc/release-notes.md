@page release-notes Release notes

@anchor latest
## ?? June 2026

Release of libcwd-2.0.0.

This is the first released of libcwd under the MIT license!

After that, the biggest change is probably that support for memory allocation debugging has been removed.

Other changes include,

- libcwd_r.so was renamed to libcwd.so
- Debug(main_reached()) must now be called at the top of main. This also does
what check_configuration() did, which therefore no longer exists.
- Complete rewrite of ELF/DWARF debug information loading code; dc::bfd was renamed to dc::elfutils.

For a more detailed list, see the NEWS file.

---
## 2 November 2024

libcwd-1.2.1 is released.

---
## 28 March 2021

libcwd-1.1.2 is released.
Fixed a bug in reading the DWARF4 .debug_info header, leading under certain circumstances
to a crash in `objfile_ct::load_dwarf()` before reaching main when having configured
libcwd with `--enable-location` (autotools) or `‑DEnableLibcwdLocation:BOOL=ON` (cmake).
Added support for cmake.
The recommended way to use libcwd in a cmake project is now to use
[gitache](https://github.com/CarloWood/gitache) and [cwds](https://github.com/CarloWood/cwds).

---
## 7 Sept 2019

libcwd-1.1.1 is released.
All occurances of `trylock` have been renamed to `try_lock`.
This breaks interface API backwards compatibility, but it allows one to use `std::mutex`
for `DebugObject::set_ostream`.
Fixed address/location lookup for PIE executables (most executables are, these days — which caused
address to location lookups to fail for addresses inside the main executable).
Added support for `aligned_alloc`, yet another allocation library function that was added
around 2011 — I just never tried to use it before now (and nobody else reported missing support either).

---
## 22 Sept 2016

libcwd-1.0.6 is released.
This version adds support for gcc version 5.x and 6.x as well as DWARF 4.
It also fixes a problem with finding symbol _end and other problems related
to symbols due to API/ABI changes in third party libraries.
A few other stability problems where fixed as well; most notably where
a memory block was reallocated by a different thread than the thread that
originally allocated it.

This version further adds a per-invocation debug channel control by
allowing to pass a conditional to a debug channel: `Dout(dc::notice(cond), ...)`
which then will only print debug output if `cond` is true (and `dc::notice` is on).

---
## 2 Jun 2010

libcwd-1.0.4 is released.
This version fixes the problem that occurred with the initialization of libcwd
with the lastest libc (2.10) because that calls dlopen before calling malloc et al.
It also fixes a bug in the internal read/write lock implementation that theoretically
could lead to dead locking of libcwd.

---
## 28 Jul 2009

libcwd-1.0.3 is released.
This version fixes a vew bugs.
The most important bug being fixed involved a crash
while accessing a deleted `memblk_map_ct` in `search_in_maps_of_other_threads`.

If there are still undeleted memory allocations when a thread is destroyed, then its
`thread_ct` object is not removed from the thread list that `search_in_maps_of_other_threads` searches
but it is flagged as a 'zombie'. As soon as the last memory allocation is deleted
its `memblk_map_ct` is deleted (cleanup) but the `thread_ct` was not
removed from the thread list, causing a crash the next call to `search_in_maps_of_other_threads`.

---
## 23 Jan 2008

libcwd-1.0.0 is released.
This version adds support for sparc64. All configure options now work also
on 64bit. Support for the latest SVN version of gcc (4.3) was updated.

An important bug has been fixed for the threaded case:
libcwd_r uses several pthread_mutexattr_t objects, but never initialized those.
This resulted in uninitialized memory being used resulting in random
mutex attributes. I believe this to be the reason that gcc-3.x locked
up on me. This version of gcc is therefore now supported again.

Last but not least, as you see the version has been
bumped to 1.0.0! That means that the API is now holy. The SONAME
major version has been set to 1 (it was 99 before). I do not expect
this to change again (I will not change the existing API, or remove
interfaces-- not that this has happened in the past years anyway).

---
## 7 Jul 2007

libcwd-0.99.47 is released.
This version adds support for x86_64. Yes, yes, I have a 64 bit machine
myself now, finally. It wasn't much work at all; so, to all those people
who have mailed me in the past about support for 64 bit: WHY DO YOU THINK
IT IS OPEN SOURCE?

---
## 21 May 2007

libcwd-0.99.46 is released.
A new configure option was added, `--enable-glibcxx-debug`,
that will cause libcwd to be compiled with
[‑D_GLIBCXX_DEBUG](https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode_using.html#debug_mode.using.mode).
When using magic numbers, but no redzone around allocations (the default),
the one to three bytes directly following the allocation till the next
alignment with a multiple of sizeof(size_t) were not checked. Thus,
a small overrun would not be detected in that case. This has been fixed.
Finally, this version was again updated to work with the current
SVN version of gcc (4.3).

---
## 9 Nov 2006

libcwd-0.99.45 is released.
This version works with gcc 4.2 (and the current 4.3 in SVN).
The library now works completely on Debian (I started to use debian myself).
Support for debug symbol packages (*-dbg on debian) was added: libcwd now
reads both, .debug/ and /usr/lib/debug/ debug files (as does gdb) when
the original library is stripped (it will not attempt to read them if
the original library still has a symbol table). libcwd's repository was
converted from CVS to SVN.

Threading support with gcc versions 3.x has been dropped (use gcc 4.x).
It might work for you, or it might deadlock. Non-threading still
works fine with gcc 3.3 or higher.

Support for libbfd (--enable-libbfd) has been completely
removed; I don't think anyone was using it anyway.

---
## 23 May 2006

libcwd-0.99.44 is released.
This release implements the possibility of 'redzones': you
can configure libcwd, using --with-redzone=x (in bytes), and it will
add (at least) x bytes above and below every allocation. See the NEWS
file in the release for more info. This has been quite a while in CVS,
now also released as tar ball.

People who build libcwd from CVS (and therefore need to run autogen.sh)
be aware: my autoconf macros are now put in a separate project called
cwautomacros (I use them for multiple projects). You will need
to download and install cwautomacros before you can configure libcwd.

Two bugs have been fixed: one that could cause a core dump during
global deinitialization (when the application terminates) and one
that could cause a COREDUMP for a self-"collision" of a shared
library at start up, namely when one tried to dlopen a library is
already linked to the application in the normal way.

---
## 13 Dec 2005

libcwd-0.99.43 is released.
This release fixes problems with the calculation of the load address
of shared libraries. On some systems, this bug resulted in not finding
a shared object for a given address, finding the wrong one, or even
core dumping on start up.

---
## 27 Nov 2005

libcwd-0.99.42 is released.
Compiler warning fix for g++ 4.x in private_allocator.h.

---
## 26 Nov 2005

libcwd-0.99.41 is released.
This release adds support for g++-4.0.2.
Addresses in libraries that are loaded on addresses lower than
the executable (yeah, I didn't know that was possible either) are now resolved.
dlopen/dlclose are now caught at link time
instead of `#defining` them, solving a bug that could cause a crash
when for example dumping memory allocations done in already dlclosed modules.
Finally, the .spec file was fixed so that
'`rpmbuild -ta libcwd-0.99.41.tar.gz`'
should work again on the latest Fedora release (that uses a new version of rpm).

---
## 31 May 2005

libcwd-0.99.40 is released.
This release adds support for g++-4.0.0.

---
## 8 Oct 2004

libcwd-0.99.39 is released.
This release fixes a few major bugs.
The most important one caused libcwd to crash even before reaching main
when the compiler used is configured with --enable-nls, making libcwd
completely useless on for example [Gentoo](http://www.gentoo.org/).
A second important bug that was fixed is one where the shared libraries
that one uses are stripped (as is also the case on Gentoo).
In this case libcwd failed to determine the end of a shared library
because the symbol "_end" is missing and it took the end of the first
function in the library by accident as end of the library, instead of
the last function.
As a result, addresses inside such stripped libraries could not be found
by libcwd - and worse, trying to print such locations caused a core dump!

Special thanks go to Michal Grzejszczak for reporting the
problems with Gentoo; not many people take the effort to report bugs
these days, especially for a package that crashes immediately the first
time you try a 'Hello World'!  Thanks to him also Gentoo users can use libcwd
from now on.

---
## 24 Sep 2004

libcwd-0.99.38 is released.
This release adds support for gcc 3.4.2 (needed for the threaded case) and for
the latest gcc CVS version 4.0.0 (20040922).
The configuration option --disable-location has been fixed, and when using
this together with --disable-alloc you will now be able to use libcwd for
debug output on cygwin and mingw32 (no threading yet).

Along with a few other minor bugs, a rather nasty one that
appeared first on fedora core 2 machines have been fixed.  If you have been
getting the following error when starting an application linked with libcwd:

`FATAL : std::ifstream.open(""): ENOENT (No such file or directory)`

Then you should certainly upgrade as this release will fix that.

---
## 15 Jul 2004

libcwd-0.99.37 is released.
This release concentrates on improving memory leak detection.
Unfortunately it had to contain again an API change: `ooam_filter_ct` has
been renamed to `alloc_filter_ct` and `ooam_format_t` has likewise
been renamed to `alloc_format_t`.

With this release it is possible to filter the list
of printed allocations on *allocation-function* (the function from which
the allocation is done).
This makes it in principle possible to print a precise overview of leaked memory
at process termination by suppressing a list of "leaks" that are not really leaks.
Memory allocations now print the location where they are done at the moment they
are done - and not only when they are freed.
The memory allocation filter got a new flag `show_function` which causes
the mangled allocation-function to be shown (even when the sourcefile and line
number are known).  I left this deliberately in mangled form because it
is more likely to be used in a regular expression search in your debug output
log file than to be understood by a human (and you can always use c++filt to
demangle it).
Finally, the function `list_allocations_on` now returns the number
of visible, non-filtered allocations; a value larger than 0 can then be
interpreted as 'memory leak'.
A pair of functions has been added to more easily control whether third-party
allocations are invisible or not: `libcwd::set_invisible_on` and
`libcwd::set_invisible_off`.
Memory allocated while this flag is set will be invisible and the MALLOC
showing the allocation will have '(invisible)' appended.
This line now also always shows the location where the allocator is called from.

This release fixes the --disable-alloc configure flag and adds
support for gcc version 3.4.1 (needed for threaded applications).

---
## 24 Jun 2004

libcwd-0.99.36 is released.
A very minor bug fix release.

---
## 23 Jun 2004

libcwd-0.99.35 is released.
This release fixes a bug related to making memory allocations invisible.
Before it was not possible to make an allocation invisible that was allocated
with realloc(3) due to an assertion failure.
Library authors who use libcwd will need to carefully read the NEWS file
and [The Custom debug.h File / Libraries](group__chapter__custom__debug__h.html#libraries)
paragraph in the reference manual, because the way that it has been set up has been changed.

---
## 27 May 2004

libcwd-0.99.34 is released.
Although this is mainly a bug fix release, it also contains a major API change:
the namespace `libcw::debug` was renamed to namespace `libcwd`.
Libcwd is now an official debian package and see, as a result I got some bug reports (finally!).
It turned out that libcwd had problems with certain shared libraries that use more than 256 entries
in the .debug_abbrev section.  Finally, there turned out to exist two global initialization
order fiasco bugs for the non-threaded case; one showing on debian and one showing on gentoo.
All reported bugs have been fixed.

---
## 23 Apr 2004

libcwd-0.99.33 is released.
Added support for gcc version 3.4.0.  The latest gcc CVS version 3.5 is working again.
Two new features have been added:
[`debug_alloc(ptr)`](group__chapter__gdb.html#a1)
and [`debug_watch(ptr)`](group__chapter__gdb.html#a0).
Both can only be called from within gdb.  The first prints information about the memory
allocation (if any) under the pointer `ptr` and the second causes gdb to stop
as soon as the memory allocation under `ptr` is deleted or freed.

---
## 22 Feb 2004

libcwd-0.99.32 is released.
This release fixes a possible deadlock during initialization, in the threaded case.
The latest gcc CVS version 3.4 and 3.5 are working again.
Two new features have been added:
[`read_rcfile()`](group__chapter__rcfile.html)
and [`attach_gdb()`](group__chapter__attach__gdb.html).
The first reads initialization from an rcfile (allowing to set which debug channels
should be on or off for example) and the second opens an xterm with an attached gdb session in it,
allowing you to start to debug the application from that point (especially handy for threaded
applications).

---
## 28 Jun 2003

libcwd-0.99.31 is released.
This release really works with glibc-2.3.2 and RedHat 9.
I missed the fact that RedHat 9 supports NPTL
([Native POSIX Threads Library](http://people.redhat.com/drepper/nptl-design.pdf))
and TLS ([Thread Local Storage](http://people.redhat.com/drepper/tls.pdf))
(TLS needs at least g++ 3.3 or the version of 3.2.3 that comes with RedHat 9).
This release reimplements the way that libcwd deals with TSD (Thread Specific Data)
and now *really* works with NPTL.
Only the version of NPTL that comes with RedHat 9 was tested though (0.35, the current
[development version of NPTL](http://people.redhat.com/drepper/nptl/) is 0.50).
You will need at least g++ 3.2.1 in order to get threading to work when you
have an NPTL enabled glibc, this is due to a bug in NPTL-0.35 but that bug
doesn't cause any problems with g++ 3.2.1 and higher.

---
## 21 May 2003

libcwd-0.99.30 is released.
This release works with glibc-2.3.2 and RedHat 9.
I found that the glibc that is distributed with rawhide is broken.
A few threads related bug have been fixed among which a serious one
that caused allocations done between two `list_allocations_on()`'s
using the same filter (or none at all, which uses the default filter) to be
randomly filtered (depending on an uninitialized boolean).

---
## 3 May 2003

libcwd-0.99.29 is released.
This release contains a few large changes.
All the header files have been moved from `libcw/` to `libcwd/`.
Compiler version 3.x is now required (2.95 and 2.96 didn't support stringbuffers
well enough).
All allocations inside `Dout()` et al are now non-internal!
Finally there were a few minor bug fixes.
Please read the `NEWS` file as usual, for more details.

---
## 12 February 2003

libcwd-0.99.28 is released.
This release adds support for `new(std::size_t, std::nothrow_t const&)`
allowing it to be used with gtkmm-2.0 among others.
The demangler was fixed to match the mangling of g++ 3.1, which uses a slightly
different way to mangle constant member functions than g++ 3.0 did.
The decoding of line numbers has once again been improved and is now perfect
(assuming you don't use a compiler that generates broken debug information).
Finally, this release fixes a bug where dlopen() could only read
libraries with an absolute path, failing for libraries that
reside somewhere in `LD_LIBRARY_PATH`.

---
## 16 November 2002

libcwd-0.99.27 is released.
This release fixes a few minor bugs.  Well, minor unless you run into them of course: libcwd would core dump
when reading a shared library whose last compilation unit did not contain a .debug_line section.
And linking with a libcwd that was configured with --disable-alloc would lead to an undefined reference.
Finally, demangling of _GLOBAL_* functions have been fixed.

---
## 13 September 2002

libcwd-0.99.26 is released.
This release contains some bug fixes and a new 'overview of allocated memory' filter class extension:
`ooam_filter_ct::hide_unknown_locations(bool)`.

---
## 5 September 2002

libcwd-0.99.25 is released.
This release fixes a few major bugs: internal memory leaks in libcwd
itself (now garanteed leak free!), and the demangle routines weren't
thread safe when you were using compiler version 2.95.x or 2.96.
Also, the library was erroneously not compiled with -O, making it
about twice as slow (still very fast).
The function `list_allocations_on()` didn't work at all for
other debug objects; only `list_allocations_on(libcw_do)` would work.
Under certain circumstances (linking with a third party C++ library
that got initialized first and whose first allocation is via
std::allocator) would lead to a dead lock during initialization.
Finally, `Channel::get_label()` has been changed to be zero
terminated.  The fact that it was not (deliberately), wasn't very
clearly documented.

---
## 19 August 2002

libcwd-0.99.24 is released.
This release fixes multi-threading problems among which writing continued
debug output: the label of each output line will now really always start
on a new line.

---
## 18 July 2002

Today an excellent [review](http://www.kuro5hin.org/story/2002/7/18/3313/01429) of C++ tools appeared on the front page of
[kuro5hin.org](http://www.kuro5hin.org/) in which libcwd
received positive critics.

---
## 17 July 2002

libcwd-0.99.23 is released.
This release adds two possibilities to the 'overview of allocated memory' filter class:
`ooam_filter_ct::hide_sourcefiles_matching(std::vector<std::string> const&)`
and `ooam_filter_ct::hide_untagged_allocations(bool)`.

You now can pass a filter to a `marker_ct` to
specify which allocations are to be considered outside the marker
area as well as how to list the allocations that were leaked.
Three minor bugs have been fixed: two related to the use
of dlclose() and one that caused the full path of location source
files to be incomplete when using gcc 3.x.

---
## 21 June 2002

libcwd-0.99.22 is released.
Fixes a horrible bug that was introduced in 0.99.20, causing
any realloc() to crash when not having used an AllocTag for it.
Did not run into this before because all of the testsuite uses
AllocTag for reallocated blocks, and libstdc++ doesn't use realloc
at all.
I now ran into it because I am playing with libgtkmm.

---
## 18 June 2002

libcwd-0.99.21 is released.
This release adds support for g++ 3.1.
In the previous two versions there was an include missing
causing libcwd not to compile on SuSe and Mandrake.
The administration of allocated memory is now kept
in an STL map *per* thread instead of one big global map.
The advantage of that is that in general a
call to new/malloc etc. will not cause a thread to
have to wait for other threads anymore.

---
## 24 May 2002

libcwd-0.99.20 is released.
This release fixes three (possible) deadlocks in `list_allocations_on`,
one in `dlopen` (when an error occurred during loading),
and one during core dumping (`COREDUMP`) in
`pthread_kill_other_threads_np`.
Furthermore a bug was fixed in relation to the use of the control flag
`error_cf` as that was using the non-reentrant
libc function `strerror` instead of
`strerror_r`, causing libcwd likely to complain
about freeing an 'internal' allocation (which means that the user (or the
system) is freeing memory that was allocated by libcwd, and libcwd likes
to take care of its own allocations).

Support for non-threaded applications on solaris 2.8 has been added as well.
Please contact me if you are interested in making threads work too on this OS.
The same holds for FreeBSD.

---
## 23 April 2002

libcwd-0.99.19 is released.
This release fixes a deadlock during the initialization of threaded applications
that use g++ version 2.96 or earlier.  There are now two libraries compiled
and installed: a thread safe version which is called libcwd_r, and the thread
unsafe version called libcwd.  Finally, a new feature is added that allows
one to filter the overview of allocated memory, removing the output related
to certain shared libraries; restricting the memory allocations shown to a
given time interval in which they were allocated; and showing the full path
of a location source file at which the allocation took place, the time at
which the allocation was made and/or the name of the shared library an allocation
belongs to.  Detailed information can be found in the [reference manual](group__group__alloc__format.html).

---
## 11 March 2002

libcwd-0.99.18 is released.
This release contains several minor documentation fixes and again an API change: the
configure option related macros (`DEBUGMALLOC, DEBUGDEBUG` etc) have been
renamed and are defined to 0 or 1 instead of being undefined or defined.
As always, read the NEWS file!

---
## 10 March 2002

A project has been created on [sourceforge](http://sourceforge.net/projects/libcwd/)
especially for libcwd.
This means that libcwd also has a new home page now (you're probably looking at it):
[http://libcwd.sourceforge.net/](http://libcwd.sourceforge.net/).

**Bookmark this page!**

---
## 18 February 2002

libcwd-0.99.17 is released.
This release fixes a few bugs related to threading.
Also a few typos in the tutorial have been fixed and a new chapter
on debugging threaded applications was added.

Errata: There is a typo in the documentation.
The environment variable isn't `LIBCWD_ALWAYS_PRINT_LOADING`
but `LIBCWD_PRINT_LOADING`.

---
## 13 February 2002

libcwd-0.99.16 is released.
Five months of non-stop, hard work, but here it finally is: libcwd is now thread-safe!
In order to get a thread-safe version you have to use the configuration option
`‑‑enable‑libcwd‑threading`.
This release also contains new documentation that was generated with
[doxygen](http://www.doxygen.org); no more slow on-line browsing,
now you can read the reference manual locally.

There are a few major API changes again (I know, I know - but if you don't
like it then wait till release 1.0).
If you are upgrading from a previous version:
**Read the NEWS file that comes with the source distribution**
for details about these API changes!

---
## 22 September 2001

libcwd-0.99.15 is released.
This release contains one API change: you now *must* define `CWDEBUG` when compiling.
Using `‑DDEBUG` will not work any longer.
A few major bugs have been fixed in this release.  Two demangle bugs, a bug related to symbol
name lookup and a compilation error (gcc-3.0 couldn't find the header file `gthr-default.h`,
which is actually a bug in the header files of the compiler).
A bug in the demangler for gcc-2.96 and earlier caused core dumps for symbols that
started with `_X...`.  Since those are used a lot in
X libs, libcwd was unusable with X windowing system applications (like those using qt).
This version of libcwd has been tested (for the first time) with a single threaded
qt application.  Another major problem was an erronous assertion in the ELF
symbol lookup code that failed on glibc-2.2.4, making libcwd unusable with that version.

---
## 28 August 2001

libcwd-0.99.14 is released.
Nobody did notice it (nobody mailed me about it), but under certain circumstances the previous version could
horribly fail in determining the correct line number of a location.  This version should fix that!
There is more news however, we have a new developer for libcwd!  Eric is an experienced coder who I've
known for about eight months now (on our IRC channel `#g++` on Undernet).  I've already been enjoying many
fruitful discussions with him before, and now he joined the coding forces in order to attack the thread-safety
problem.  We're hoping to make libcwd thread-safe in the foreseeable future.  In the meantime you
can see what we're doing using CVS and the unstable alpha branch:`branch-threading`.

---
## 20 August 2001

libcwd-0.99.13 is released.
New in this release is support for dynamically loaded shared object files (`dlopen()`).
Also new is native support for the DWARF 2 debug format (this is used when you compile with `-ggdb`
or `-gdwarf-2`.
It is also used for `-g` when DWARF is the native debug format, as is the case on IRIX 6).
The advantage of DWARF is that it is much faster to load: it is very compact and stores the line number information
in a seperate section so that we don't have to load other debug info.
In the case of stabs (the native debug format of linux) we need to load *all* debug information.
You'll notice that libcwd now prints "Loading debug info from ..." under all circumstances when an application is started.
It uses debug channel `dc::bfd` and in most cases *before* `main()`
is reached.  This is achieved by forcing the debug object as well as `dc::bfd` to *on*
prior to loading the debug information from the object files.  This was disabled in previous versions because it's a bit
tricky: at this point the debug object might not be initialized yet!  Previously, only a snap shot version of g++ that
I kept around caused a crash because of this (it depends highly on the order in which global objects are initialized), but now
I made a few changes that make me feel confident enough to enable it by default.  If you still get a crash, or if you want
to disable this printing for other reasons, then you can `#undef ALWAYS_PRINT_LOADING` in
`src/libcwd/bfd.cc`.

This release contains **two API changes** - and rather annoying ones I am afraid:
Firstly, the header file `<sys.h>` has been renamed to `<sysd.h>`.
Please manually remove the old `<sys.h>` if you install libcwd over an old install: that is the
only way to be sure that you are not accidently including it somewhere.
Secondly, the debug channel `dc::warning` is turned on by default - which will cause a core dump
if you try to turn it on yourself at the start of `main()`: You have to remove an explicit
`Debug(dc::warning.on())` if you have one.

---
## 1 August 2001

libcwd-0.99.12 is released.
This release fixes the bug that libcwd was accessing recently freed memory.
In all that time this never lead to a crash for me, amazing.

---
## 31 July 2001

libcwd-0.99.11 is released.
Debug channel `dc::stabs` was renamed *back* to
`dc::bfd`.  Possible confusion was now solved
differently by renaming the configure option `--enable-libcwd-bfd`
into `--enable-libcwd-libbfd`.

---
## 31 July 2001

libcwd-0.99.10 is released.
This release comes with two major changes.  `malloc(3)` and friends are now
exported with external "C" linkage instead of being a macro; this means that also allocations done from
other shared libraries like libc are caught now.  Most notably, this fixes the bug that
`strdup(3)` couldn't be used (thanks for the feedback I now got!).
This release doesn't depend anymore on GNU's `libbfd`; it is now capable of understanding stabs debug
info read from ELF32 object files directly.  You will need to use the configure option `--enable-libcwd-bfd`
to get the old behaviour back and still link with `libbfd`.  Note that due to the viral nature
of the GPL (that doesn't allow one to link a program with a GPL-ed library and then distribute it under
any other license but the GPL (unlike the LGPL and QPL)), you are not allowed to *distribute* executables
that are linked against `libbfd` because you could only do so under the GPL and that violates the license of libcwd (QPL).
I trust that this is not a problem as these are purely test-executables.

---
## 9 July 2001

libcwd-0.99.9 is released.
This release comes with a brand new demangler for the new C++ ABI as promised.
I am happy to have learned that there exists interest in libcwd in the Open Source community: the maintainer
of gdb wrote me that he translated the new demangler into C and will incorporate it into GNU libiberty
and gdb (the current demangler in libiberty doesn't handle qualifiers correctly in all cases).
The maintainer of clisp expressed his interest in the bfd.cc file in order to be able to print
backtraces in case of internal errors.

I have very little feedback from users of libcwd itself though.
If there is anything you would like to be added, or when you find a bug, **mail me**.
*It won't get fixed when you don't tell me about it!*
But if you are just a happy user without any complaint, I'd really love to hear from you too!
Click [here](http://www.xs4all.nl/~carlo17/anti/spam/bots/dont/like/deep/dirs/mail2.html?libcwd) and drop me mail!

---
## 29 May 2001

libcwd-0.99.8 is released.
Finally, after a long delay a new release - and as promised one that works with libstdc++ version 3!
This release still lacks a demangler for the new C++ ABI however (that is part of g++-2.97 and higher), which
will be added to the next release.

Making libcwd suitable for libstdc++-v3 wasn't an easy task.  One of the major problems is
that during the writing of debug output (for example `operator<<(ostream&, int)`)
memory is being allocated under certain circumstances by libstdc++-v3.  While it is unacceptable that
due to writing debug output other debug output is generated, there were only two options: 1) turning off
the `malloc` and `bfd` debug channels but still storing all
memory allocations in memory allocation overview, or 2) making all memory allocations done from a
`Dout` *internal*, disabling any checks.  With in mind that the existance of
debug code shouldn't matter (after all, it won't be in the final production version of an application) it follows
that no memory should be allocated in the first place as part of a `Dout`
and when it *is* it isn't important if it leaks or not - I decided to go for solution two because that
uses less cpu.

Other changes involve mostly a rewrite of certain parts that were depending on a specific GNU implementation
of libstdc++, using the now available standard.
While most of these changes are invisible, there was a small change to the interface of [`type_info_of`](bfd.html#type_info_of)
resulting at the same time in preserving a top-level `const` qualifier as well as faster code
(with g++-3.0 that is; older versions of g++ are still so much broken that slower work-arounds were needed).
The only kludge left in libcwd at the moment is the detection of when the standard streams are initialized.  I blame the need
for this kludge on a bug in libstdc++-v3 and have reported it as such; perhaps also this can be gotten rid of in the
future.

At the moment of writing two problems are already known with 0.99.8:

Using a libbfd on a 32bit machine
that was compiled with 64bit target support will fail to work with libstdc++-v3 when the latter is compiled without
support for `long long` (`long long` is *not* a part of the
C++ standard, while it *is* part of C99.  libbfd being a C library, this results in problems).  I added
a test for this to 0.99.8 but accidently forgot to test for libstdc++-v3 causing libcwd to fail always with g++-2.95.x!
If you get this error:
*libbfd is compiled with 64bit support on a 32bit host, but your libstdc++ is not compiled with support for long long.*
and you are using g++-2.95.x then just remove the `#error` line from bfd.cc and it should compile just fine.

Finally, it is known that many operating systems can't deal with shared C++ libraries and global objects.  While this
means that those operating systems are broken, I decided to fix this problem by removing all global objects in the next
release.  So, if libcwd core dumps on you for even very simple programs then please try 0.99.9 when it gets released.

---
## 2 March 2001

libcwd-0.99.7 is released.
This release includes a work-around for the compiler crash in `lockable_auto_ptr.h`
that occurred with g++-2.95.x.
There are a few other changes however; most notably, the support for the macro `Dout_vform`
has been removed because `ostream::vform` isn't conforming to the Standard and is gone in
libstdc++ version 3.
Note that this release still doesn't work with libstdc++ version 3, hopefully the next release will.

---
## 10 February 2001

libcwd-0.99.6 is released.
This release deals with namespace issues and therefore contains a few major API changes.
The most important changes follow.  The macro DEBUG has been renamed to CWDEBUG because
the former was too general and collided with other libraries.
All functions have been moved to namespace `libcwd`;
this will break compilation of programs that use earlier versions of libcwd,
unless one includes a "`using namespace libcwd;`"
line in the source files using more than just `Dout` and
`Debug`.
Four functions have not only been moved to the new namespace but were also renamed.
Apart from this all, also the interface to BFD has been redesigned; among others the
structure `location_st` has been replaced by a class `Location`.  Also here
functions have been renamed or even removed.
*Please read the **NEWS** file contained in the source distribution for details.*
A new template function has been added: `cwprint_using`.
This utility allows one to easily print objects using a method of the object rather
than `operator<<`.  Finally, two bugs have been
fixed: one in the demangler which still couldn't demangle const member functions, and
a bug that could cause a core dump when memory was allocated from the constructor of
a global object that was initialized before libcwd was initialized, this has only been
reported for solaris.

---
## 29 January 2001

libcwd-0.99.5 is released.
This release contains mostly bug fixes, especially for the demangler.
Otherwise it mainly contains support for the compiler that is released with
RedHat-7.0: gcc-2.96.
Version 0.99.5 contains two API changes: the prototype of `libcw_bfd_pc_location`
was changed in order to get rid of the *named return values* gcc extension that has been
deprecated in g++-2.96 and higher.
Secondly, `DoutFatal` doesn't have a default
debug channel anymore and `dc::fatal`,
which was the default in prior releases, must explicitly be specified.

---
## 11 October 2000

libcwd-0.99.4 is released.
This release comes with a testsuite and again allows Linux users to
just link their application with `-lcwd`
(without the need to also specify `-lbfd -liberty`).
All reported bugs (two) have been fixed, as well as a third that showed
up while I wrote the testsuite: a locked `lockable_auto_ptr`
does *not* transfer ownership anymore when using the assignment operator.

---
## 13 September 2000

libcwd-0.99.3 is released.
This release includes an example showing how to use libcwd in
an `[autoconf](http://sources.redhat.com/autoconf/)`/`[automake](http://sources.redhat.com/automake/)` project.
**Please note** that, due to constraints of libtool, your programs need to be linked with
`-lcwd -lbfd -liberty` now (as opposed to just `-lcwd`).
Support for Solaris has been added.
The autoconf/automake configuration setup has been improved;
also the configuration on FreeBSD should work out of the box now (assuming you have binutils installed).
Finally, a `.spec` file for building an RPM has been added to this release.

---
## 29 August 2000

libcwd-0.99.2 is released.  Added autoconf/automake/libtool for configuration.
This obsoletes the need for the [prototype](http://www.xs4all.nl/~carlo17/prototype/index.html) package.
The source file `demangle.cc` was completely rewritten and now supports demangling of
[symbols](bfd.html#demangle) as well as types.

---
## 15 August 2000

libcwd-0.99.1 is released.  Apart from a few minor bug fixes, support for FreeBSD has been added!

---
## 08 August 2000

The web site has been improved in order to give new visitors a quicker overview:
libcwd now has its own home page.  A page with features and "screenshots" was added.

---
## 04 August 2000

First public announcement of libcwd on freshmeat.

---
## 23 July 2000

First public release of libcwd on sourceforge.
