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
allocator \
stabs \
lines

CLEANFILES=$(EXTRA_PROGRAMS)

hello_world_SOURCES = hello_world.cc
location_SOURCES = location.cc
syslog_SOURCES = syslog.cc
initbug_SOURCES = initbug.cc initbug_GlobalObject.cc
dout_alloc_SOURCES = dout_alloc.cc
mangling_SOURCES = mangling.cc
allocator_SOURCES = allocator.cc
stabs_SOURCES = stabs.cc
lines_SOURCES = lines.cc

threads_CXXFLAGS = -DCWDEBUG -D_REENTRANT $(DEBUGOPTS) $(TESTOPTS) $(EXTRAOPTS) $(WARNOPTS)
threads_SOURCES = threads.cc
threads_LDADD = ../libcwd_r.la

threads2_CXXFLAGS = -DCWDEBUG -D_REENTRANT $(DEBUGOPTS) $(TESTOPTS) $(EXTRAOPTS) $(WARNOPTS)
threads2_SOURCES = threads2.cc
threads2_LDADD = ../libcwd_r.la

threads3_CXXFLAGS = -DCWDEBUG -D_REENTRANT $(DEBUGOPTS) $(TESTOPTS) $(EXTRAOPTS) $(WARNOPTS)
threads3_SOURCES = threads3.cc
threads3_LDADD = ../libcwd_r.la

threads4_CXXFLAGS = -DCWDEBUG -D_REENTRANT $(DEBUGOPTS) $(TESTOPTS) $(EXTRAOPTS) $(WARNOPTS)
threads4_SOURCES = threads4.cc
threads4_LDADD = ../libcwd_r.la

noinst_HEADERS = sys.h debug.h sys.h syslog_debug.h threads_debug.h initbug_GlobalObject.h

LDADD = ../libcwd.la
full_top_builddir := $(shell (cd $(top_builddir); pwd))
LDFLAGS = @LIB_THREADS@ -Wl,-rpath -Wl,$(full_top_builddir)/.libs

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
TESTOPTS = @TESTOPTS@
CXXFLAGS = -DCWDEBUG $(DEBUGOPTS) $(TESTOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I. -I$(top_builddir)/include -I$(srcdir)/../include
DEFAULT_INCLUDES =

MAINTAINERCLEANFILES = Makefile.in

stabs.o: stabs.cc
	$(CXXCOMPILE) -O2 -c $<

stabs.lo: stabs.cc
	$(LTCXXCOMPILE) -O2 -c $<
