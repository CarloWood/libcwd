## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_PROGRAMS = \
hello_world \
syslog \
initbug \
dout_alloc \
mangling

CLEANFILES=$(EXTRA_PROGRAMS)

hello_world_SOURCES = hello_world.cc
syslog_SOURCES = syslog.cc
initbug_SOURCES = initbug.cc initbug_GlobalObject.cc
dout_alloc_SOURCES = dout_alloc.cc
mangling_SOURCES = mangling.cc

include_HEADERS = sys.h debug.h sys.h syslog_debug.h initbug_GlobalObject.h

LDADD = ../libcwd.la
full_top_builddir := $(shell (cd $(top_builddir); pwd))
LDFLAGS = -Wl,-rpath -Wl,$(full_top_builddir)/.libs

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I. -I- -I$(top_builddir)/include -I$(srcdir)/../include -DCWDEBUG

MAINTAINERCLEANFILES = Makefile.in
