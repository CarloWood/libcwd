## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libutils.la

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I- -I$(top_builddir)/include -I$(srcdir)/../include -DCWDEBUG
LIBS=

libutils_la_SOURCES = \
bcd2str.cc \
buf2str.cc \
char2str.cc \
exec_prog.cc

EXTRA_DIST = perf.cc

MAINTAINERCLEANFILES = Makefile.in
