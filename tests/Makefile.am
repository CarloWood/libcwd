## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_PROGRAMS = \
hello_world \
syslog

CLEANFILES=$(EXTRA_PROGRAMS)

hello_world_SOURCES = hello_world.cc
syslog_SOURCES = syslog.cc

include_HEADERS = syslog_debug.h

LDADD = ../libcwd.la

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I- -I$(top_builddir)/include -I$(srcdir)/../include -DDEBUG

MAINTAINERCLEANFILES = Makefile.in
