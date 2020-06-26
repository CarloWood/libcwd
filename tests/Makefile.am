## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_PROGRAMS = \
threads \
threads2 \
threads3 \
threads4 \
hello_world \
location \
syslog \
initbug \
dout_alloc \
mangling \
stabs \
lines \
inlined \
memalign

CLEANFILES=$(EXTRA_PROGRAMS)

hello_world_CXXFLAGS = -g
hello_world_SOURCES = hello_world.cc

location_CXXFLAGS = -g
location_SOURCES = location.cc

syslog_CXXFLAGS = -g
syslog_SOURCES = syslog.cc

initbug_CXXFLAGS = -g
initbug_SOURCES = initbug.cc initbug_GlobalObject.cc

dout_alloc_CXXFLAGS = -g
dout_alloc_SOURCES = dout_alloc.cc

mangling_CXXFLAGS = -g
mangling_SOURCES = mangling.cc

lines_CXXFLAGS = -g
lines_SOURCES = lines.cc

memalign_CXXFLAGS = -g
memalign_SOURCES = memalign.cc

stabs_CXXFLAGS = -O2
stabs_SOURCES = stabs.cc

inlined_CXXFLAGS = -g -finline
inlined_SOURCES = inlined.cc inlined_main.cc

threads_CXXFLAGS = -DLIBCWD_THREAD_SAFE -pthread @CXXFLAGS@
threads_SOURCES = threads.cc
threads_LDADD = ../libcwd_r.la

threads2_CXXFLAGS = -DLIBCWD_THREAD_SAFE -pthread @CXXFLAGS@
threads2_SOURCES = threads2.cc
threads2_LDADD = ../libcwd_r.la

threads3_CXXFLAGS = -DLIBCWD_THREAD_SAFE -pthread @CXXFLAGS@
threads3_SOURCES = threads3.cc
threads3_LDADD = ../libcwd_r.la

threads4_CXXFLAGS = -DLIBCWD_THREAD_SAFE -pthread @CXXFLAGS@
threads4_SOURCES = threads4.cc
threads4_LDADD = ../libcwd_r.la

noinst_HEADERS = sys.h debug.h sys.h syslog_debug.h threads_debug.h initbug_GlobalObject.h hello_world_debug.h

LDADD = ../libcwd.la
full_top_builddir := $(shell (cd $(top_builddir); pwd))
LDFLAGS = @LIB_THREADS@ -Wl,-rpath -Wl,$(full_top_builddir)/.libs

CXXFLAGS = -DCWDEBUG
AM_CPPFLAGS = -I. -I$(srcdir)/../include -I$(top_builddir)/include -I$(top_builddir)/include/libcwd
DEFAULT_INCLUDES =

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
