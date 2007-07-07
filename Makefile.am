## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = @ACLOCAL_CWFLAGS@

SUBDIRS = include utils . tests testsuite cutee

EXTRA_DIST = LICENSE.QPL README.SVN README.threads README.FreeBSD README.openbsd README.solaris \
	README.cygwin README.mingw32 README.PCH nodebug.h depcomp libcwd.pc.in libcwd_r.pc.in \
	libcwd.lsm libcwd.spec valgrind.supp
DISTCLEANFILES = libcwd_r-@VERSION@.tar.gz stamp-h.in

VERSIONINFO=@VERSIONINFO@
CXXFLAGS = @CXXFLAGS@
PCHOPTS = @PCHOPTS@
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
DEFAULT_INCLUDES =

if ENABLETHREADING
LIBCWD_R = libcwd_r.la
else
LIBCWD_R =
endif
if ENABLENONTHREADING
LIBCWD = libcwd.la
else
LIBCWD =
endif

dist_pkgdata_DATA = libcwdrc
lib_LTLIBRARIES = $(LIBCWD) $(LIBCWD_R)

if ENABLENONTHREADING
libcwd_la_CXXFLAGS = $(PCHOPTS) -DCWDEBUG
libcwd_la_SOURCES = \
	elfxx.cc \
	environ.cc \
	bfd.cc \
	debug.cc \
	debugmalloc.cc \
	private_allocator.cc \
	demangle3.cc \
	alloc_filter.cc \
	strerrno.cc \
	type_info.cc \
	function.cc \
	compilation_unit.cc
libcwd_la_LDFLAGS = -version-info $(VERSIONINFO) -no-undefined
libcwd_la_LIBADD = utils/libutils.la
endif

if ENABLETHREADING
libcwd_r_la_CXXFLAGS = $(PCHOPTS) -DCWDEBUG -DLIBCWD_THREAD_SAFE -D_REENTRANT -pthread
libcwd_r_la_SOURCES = \
	elfxx.cc \
	environ.cc \
	bfd.cc \
	debug.cc \
	debugmalloc.cc \
	private_allocator.cc \
	demangle3.cc \
	alloc_filter.cc \
	strerrno.cc \
	type_info.cc \
	function.cc \
	compilation_unit.cc \
	threading.cc \
	wrapcnclpnts.cc
libcwd_r_la_LDFLAGS = -version-info $(VERSIONINFO) -no-undefined $(LIB_THREADS_SHARED)
libcwd_r_la_LIBADD = utils/libutils_r.la
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcwd.pc libcwd_r.pc

$(pkgconfig_DATA): config.status

# --------------- Maintainer's Section

dist-hook: documentation
	chmod u+w $(distdir)/Makefile.in
	cat $(distdir)/Makefile.in | \
	  sed -e 's/^\(CONFIG_CLEAN_FILES =.*\) libcwd\.spec libcwd\.lsm\(.*\)/\1\2/' > $(distdir)/Makefile.in.new && \
	  mv $(distdir)/Makefile.in.new $(distdir)/Makefile.in
	cp -pr $(top_builddir)/documentation $(distdir)
	find $(distdir)/documentation -name .svn -print | xargs rm -rf
	find $(distdir)/documentation/tutorial \( -name 'core*' -o -name '*-bin' -o -name '*.o' \) -print | xargs rm -rf
	rm -f $(distdir)/documentation/tutorial/Makefile $(distdir)/documentation/styles/Makefile
	mkdir $(distdir)/example-project
	mkdir $(distdir)/example-project/m4
	for i in $$(cd $(srcdir)/example-project && svn stat --verbose --non-interactive | sed -re 's/.*[[:space:]]//'); do \
	  test -f $(srcdir)/example-project/$$i && cp -p $(srcdir)/example-project/$$i $(distdir)/example-project/$$i; \
	done
	rm -f $(distdir)/example-project/debug.h.maintainer
	cp -p $(srcdir)/example-project/debug.h $(distdir)/example-project
	mkdir $(distdir)/debian
	for i in $$(cd $(srcdir)/debian && svn stat --verbose --non-interactive | sed -re 's/.*[[:space:]]//'); do \
	  test -f $(srcdir)/debian/$$i && cp -p $(srcdir)/debian/$$i $(distdir)/debian/$$i; \
	done
	chmod +x $(distdir)/install-sh

distclean-local:
	rm -rf rpm $(srcdir)/autom4te.cache
	if test ! -e $(top_builddir)/NEWS; then \
	  rm -rf documentation; \
	else \
	  find documentation/tutorial \( -name 'core*' -o -name '*-bin' -o -name '*.o' \) -print | xargs rm -rf; \
	fi
	if test -d example-project; then \
	  (cd example-project; rm -rf $(srcdir)/autom4te.cache .deps $(srcdir)/aclocal.m4 config.h $(srcdir)/config.h.in \
	   config.log config.status $(srcdir)/configure Makefile $(srcdir)/Makefile.in stamp-h1 program *.o); \
        fi

MAINTAINERCLEANFILES = $(srcdir)/aclocal.m4 $(srcdir)/config.h.in $(srcdir)/configure $(srcdir)/Makefile.in \
	$(srcdir)/install-sh $(srcdir)/missing $(srcdir)/mkinstalldirs $(srcdir)/config.guess $(srcdir)/config.sub \
	$(srcdir)/ltmain.sh $(srcdir)/stamp-h.in $(srcdir)/ChangeLog tags $(srcdir)/example-project/debug.h \
	$(srcdir)/depcomp troep*

@MAINTAINER_MODE_TRUE@include maintMakefile
