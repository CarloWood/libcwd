## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libutils.la

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = -DCWDEBUG $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I$(srcdir)/../include -I$(top_builddir)/include
DEFAULT_INCLUDES =
LIBS=

libutils_la_SOURCES = \
buf2str.cc \
char2str.cc \
exec_prog.cc

MAINTAINERCLEANFILES = Makefile.in
