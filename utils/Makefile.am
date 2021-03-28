## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

CXXFLAGS = @CXXFLAGS@
# The last include is so '#include "config.h"' works for the generated config.h.
# Once installed that will work because it is in the same directory.
AM_CPPFLAGS = -I$(srcdir)/../include -I$(top_builddir)/include -I$(top_builddir)/include/libcwd
EXTRA_DIST = CMakeLists.txt
DEFAULT_INCLUDES =
LIBS=

if ENABLETHREADING
LIBUTILS_R = libutils_r.la
else
LIBUTILS_R =
endif
if ENABLENONTHREADING
LIBUTILS = libutils.la
else
LIBUTILS =
endif

noinst_LTLIBRARIES = $(LIBUTILS) $(LIBUTILS_R)

if ENABLENONTHREADING
libutils_la_CXXFLAGS = -DCWDEBUG
libutils_la_SOURCES = buf2str.cc char2str.cc exec_prog.cc match.cc rcfile.cc attach_gdb.cc
endif

if ENABLETHREADING
libutils_r_la_CXXFLAGS = -DCWDEBUG -DLIBCWD_THREAD_SAFE -pthread
libutils_r_la_SOURCES = buf2str.cc char2str.cc exec_prog.cc match.cc rcfile.cc attach_gdb.cc
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
