include $(srcdir)/cwm4/root_makefile_top.am

DEFAULT_INCLUDES =
# The last include is so '#include "config.h"' works for the generated config.h.
# Once installed that will work because it is in the same directory.
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir)/include/libcwd

SUBDIRS = @CW_SUBDIRS@ include utils . tests testsuite cutee doc
DIST_SUBDIRS = include utils tests testsuite cutee doc

EXTRA_DIST += LICENSE.QPL README.threads README.FreeBSD README.openbsd README.solaris \
	README.cygwin README.mingw32 nodebug.h depcomp libcwd.pc.in libcwd_r.pc.in \
	libcwd.lsm libcwd.spec valgrind.supp
DISTCLEANFILES = libcwd_r-@VERSION@.tar.gz stamp-h.in

VERSIONINFO=@VERSIONINFO@
CXXFLAGS = @CXXFLAGS@

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
libcwd_la_CXXFLAGS = -std=c++17 $(PCHFLAGS) -DCWDEBUG
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
libcwd_r_la_CXXFLAGS = -std=c++17 $(PCHFLAGS) -DCWDEBUG -DLIBCWD_THREAD_SAFE -pthread
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

check:
	$(MAKE) -C testsuite -j 1 check

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcwd.pc libcwd_r.pc

$(pkgconfig_DATA): config.status

# --------------- Maintainer's Section

# Default installation directory of documentation and example-project.
DOCDESTDIR=$(DESTDIR)/usr/share/doc/libcwd

install-doc:
	test -e $(DOCDESTDIR)/documentation || mkdir -p $(DOCDESTDIR)/documentation
	cp -pr $(top_builddir)/doc/* $(DOCDESTDIR)/documentation
	rm -f $(DOCDESTDIR)/documentation/Makefile $(DOCDESTDIR)/documentation/doxygen.config
	find $(DOCDESTDIR)/documentation -name .git -print | xargs rm -rf
	find $(DOCDESTDIR)/documentation/tutorial \( -name 'core*' -o -name '*-bin' -o -name '*.o' \) -print | xargs rm -rf
	find $(DOCDESTDIR)/documentation -name '*.dot' -size 0 -print | xargs rm -f
	rm -f $(DOCDESTDIR)/documentation/tutorial/Makefile $(DOCDESTDIR)/documentation/styles/Makefile
	mkdir -p $(DOCDESTDIR)/example-project/m4
	if test -d $(srcdir)/example-project/.git; then \
	  echo > $(DOCDESTDIR)/example-project/.files; \
	  for i in $$(cd $(srcdir)/example-project && git ls-files); do \
	    if test -f $(srcdir)/example-project/$$i; then \
	      $(srcdir)/install-sh -T $(srcdir)/example-project/$$i $(DOCDESTDIR)/example-project/$$i; \
	      echo $$i >> $(DOCDESTDIR)/example-project/.files; \
	    fi \
	  done \
	elif test -r $(srcdir)/example-project/.files; then \
	  for i in $$(cat $(srcdir)/example-project/.files); do \
	    cp -p $(srcdir)/example-project/$$i $(DOCDESTDIR)/example-project/$$i; \
	  done; \
	  cp -p $(srcdir)/example-project/.files $(DOCDESTDIR)/example-project; \
	fi

dist-hook: documentation
	chmod u+w $(distdir)/Makefile.in
	cat $(distdir)/Makefile.in | \
	  sed -e 's/^\(CONFIG_CLEAN_FILES =.*\) libcwd\.spec libcwd\.lsm\(.*\)/\1\2/' > $(distdir)/Makefile.in.new && \
	  mv $(distdir)/Makefile.in.new $(distdir)/Makefile.in
	$(MAKE) DOCDESTDIR=$(distdir) install-doc
	mkdir $(distdir)/debian
	if test -d $(srcdir)/debian/.git; then \
	  echo > $(distdir)/debian/.files; \
	  for i in $$(cd $(srcdir)/debian && git ls-files); do \
	    $(srcdir)/install-sh -T $(srcdir)/debian/$$i $(distdir)/debian/$$i; \
	    echo $$i >> $(distdir)/debian/.files; \
	  done \
	elif test -r $(srcdir)/debian/.files; then \
	  for i in $$(cat $(srcdir)/debian/.files); do \
	    cp -p $(srcdir)/debian/$$i $(distdir)/debian/$$i; \
	  done; \
	  cp -p $(srcdir)/debian/.files $(distdir)/debian; \
	fi
	chmod +x $(distdir)/install-sh

distclean-local:
	rm -rf rpm $(srcdir)/autom4te.cache
	if test -d example-project; then \
	  (cd example-project; rm -rf autom4te.cache .deps aclocal.m4 config.h config.h.in \
	   config.log config.status configure Makefile Makefile.in stamp-h1 program *.o); \
        fi
	cd $(srcdir) && dh_clean || true
	rm -f $(srcdir)/build-stamp

@MAINTAINER_MODE_TRUE@include maintMakefile

include $(srcdir)/cwm4/root_makefile_bottom.am
