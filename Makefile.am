## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include utils . tests

BUILT_SOURCES = libcwd.lsm libcwd.spec
EXTRA_DIST = LICENSE.QPL README.CVS README.FreeBSD $(BUILT_SOURCES)
DISTCLEANFILES = libcwd-@VERSION@.tar.gz

VERSIONINFO=@VERSIONINFO@
EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I$(top_builddir) -I- -I$(top_builddir)/include -I$(srcdir)/include -DDEBUG

lib_LTLIBRARIES = libcwd.la

libcwd_la_SOURCES = \
	bfd.cc \
	debug.cc \
	debugdebugcheckpoint.cc \
	debugmalloc.cc \
	demangle.cc \
	printcw.cc \
	strerrno.cc \
	type_info.cc

libcwd_la_LDFLAGS = -version-info $(VERSIONINFO)

libcwd_la_LIBADD = utils/libutils.la

# --------------- Maintainer's Section

@MAINTAINER_MODE_TRUE@include maintMakefile
