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

include_HEADERS = syslog_debug.h

LDADD = ../libcwd.la

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I. -I- -I$(top_builddir)/include -I$(srcdir)/../include -DCWDEBUG

MAINTAINERCLEANFILES = Makefile.in
