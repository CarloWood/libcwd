## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include utils . tests testsuite

BUILT_SOURCES =@MAINTAINER_MODE_TRUE@ libcwd.lsm libcwd.spec
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

dist-hook:
	mkdir $(distdir)/example-project
	for i in `grep '^/' $(srcdir)/example-project/CVS/Entries | cut -d/ -f2`; do \
	  cp -p $(srcdir)/example-project/$$i $(distdir)/example-project; \
	done
	chmod +x $(distdir)/install-sh

distclean-local:
	rm -rf rpm

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure stamp-h.in Makefile.in \
	install-sh missing mkinstalldirs config.guess config.sub ltconfig ltmain.sh

@MAINTAINER_MODE_TRUE@include maintMakefile
