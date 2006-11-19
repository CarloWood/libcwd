## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

CXXFLAGS = @CXXFLAGS@
INCLUDES = -I$(srcdir)/../include -I$(top_builddir)/include
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
libutils_la_SOURCES = \
buf2str.cc \
char2str.cc \
exec_prog.cc \
match.cc \
rcfile.cc \
attach_gdb.cc
endif

if ENABLETHREADING
libutils_r_la_CXXFLAGS = -DCWDEBUG -DLIBCWD_THREAD_SAFE -D_REENTRANT -pthread
libutils_r_la_SOURCES = \
buf2str.cc \
char2str.cc \
exec_prog.cc \
match.cc \
rcfile.cc \
attach_gdb.cc
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
