## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libutils.la

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I$(srcdir)/../include -I$(top_builddir)/include -DCWDEBUG
LIBS=

libutils_la_SOURCES = \
buf2str.cc \
char2str.cc \
exec_prog.cc

MAINTAINERCLEANFILES = Makefile.in
