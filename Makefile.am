## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include utils . tests testsuite

BUILT_SOURCES =@MAINTAINER_MODE_TRUE@ libcwd.lsm libcwd.spec nodebug.h
EXTRA_DIST = LICENSE.QPL README.FreeBSD README.openbsd README.nodebug.h debian $(BUILT_SOURCES)
DISTCLEANFILES = libcwd-@VERSION@.tar.gz

VERSIONINFO=@VERSIONINFO@
EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = -DCWDEBUG $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I$(top_builddir) -I- -I$(top_builddir)/include -I$(srcdir)/include

lib_LTLIBRARIES = libcwd.la

libcwd_la_SOURCES = \
	elf32.cc \
	bfd.cc \
	debug.cc \
	debugdebugcheckpoint.cc \
	debugmalloc.cc \
	demangle.cc \
	demangle3.cc \
	cwprint.cc \
	strerrno.cc \
	type_info.cc \
	no_alloc_checking_stringstream.cc

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
	install-sh missing mkinstalldirs config.guess config.sub ltmain.sh ltconfig

@MAINTAINER_MODE_TRUE@include maintMakefile
