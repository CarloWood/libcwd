## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
INCLUDES = -I$(srcdir)/../include -I$(top_builddir)/include
DEFAULT_INCLUDES =
LIBS=

if ENABLETHREADING
LIBUTILS_R = libutils_r.la
else
LIBUTILS_R =
endif
lib_LTLIBRARIES = libutils.la $(LIBUTILS_R)

libutils_la_CXXFLAGS = -DCWDEBUG $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
libutils_la_SOURCES = \
buf2str.cc \
char2str.cc \
exec_prog.cc

if ENABLETHREADING
libutils_r_la_CXXFLAGS = -DCWDEBUG -D_REENTRANT -pthread $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
libutils_r_la_SOURCES = \
buf2str.cc \
char2str.cc \
exec_prog.cc
endif

MAINTAINERCLEANFILES = Makefile.in
