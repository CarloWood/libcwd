## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include utils . tests testsuite

BUILT_SOURCES =@MAINTAINER_MODE_TRUE@ libcwd.lsm libcwd.spec example-project/debug.h
EXTRA_DIST = LICENSE.QPL README.FreeBSD README.openbsd nodebug.h $(BUILT_SOURCES)
DISTCLEANFILES = libcwd-@VERSION@.tar.gz

VERSIONINFO=@VERSIONINFO@
EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = -DCWDEBUG $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I$(top_builddir)/include -I$(srcdir)/include
DEFAULT_INCLUDES =

lib_LTLIBRARIES = libcwd.la

libcwd_la_SOURCES = \
	threading.cc \
	elf32.cc \
	bfd.cc \
	debug.cc \
	debugmalloc.cc \
	demangle.cc \
	demangle3.cc \
	strerrno.cc \
	type_info.cc

libcwd_la_LDFLAGS = -version-info $(VERSIONINFO)

libcwd_la_LIBADD = utils/libutils.la

# --------------- Maintainer's Section

dist-hook: documentation
	cp -pr $(srcdir)/documentation $(distdir)/documentation
	find $(distdir)/documentation -name CVS -print | xargs rm -rf
	mkdir $(distdir)/example-project
	for i in `grep '^/' $(srcdir)/example-project/CVS/Entries | cut -d/ -f2`; do \
	  cp -p $(srcdir)/example-project/$$i $(distdir)/example-project; \
	done
	rm $(distdir)/example-project/debug.h.maintainer $(distdir)/example-project/macrotest.cc
	cp -p $(srcdir)/example-project/debug.h $(distdir)/example-project
	chmod +x $(distdir)/install-sh
	mkdir $(distdir)/debian
	for i in `grep '^/' $(srcdir)/debian/CVS/Entries | cut -d/ -f2`; do \
	  cp -p $(srcdir)/debian/$$i $(distdir)/debian; \
	done

distclean-local:
	rm -rf rpm

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure stamp-h.in Makefile.in \
	install-sh missing mkinstalldirs config.guess config.sub ltmain.sh ltconfig \
	config.cache config.log config.status depcomp

@MAINTAINER_MODE_TRUE@include maintMakefile
