## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include utils . tests testsuite

BUILT_SOURCES =@MAINTAINER_MODE_TRUE@ libcwd.lsm libcwd.spec .example-project_debug.h-timestamp
EXTRA_DIST = LICENSE.QPL README.FreeBSD README.openbsd nodebug.h $(BUILT_SOURCES)
DISTCLEANFILES = libcwd_r-@VERSION@.tar.gz

VERSIONINFO=@VERSIONINFO@
EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
INCLUDES = -I$(top_builddir)/include -I$(srcdir)/include
DEFAULT_INCLUDES =

if ENABLETHREADING
LIBCWD_R = libcwd_r.la
else
LIBCWD_R =
endif
lib_LTLIBRARIES = libcwd.la $(LIBCWD_R)

libcwd_la_CXXFLAGS = -DCWDEBUG $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
libcwd_la_SOURCES = \
        elf32.cc \
        environ.cc \
        bfd.cc \
        debug.cc \
        debugmalloc.cc \
        demangle.cc \
        demangle3.cc \
        strerrno.cc \
        type_info.cc
libcwd_la_LDFLAGS = -version-info $(VERSIONINFO)
libcwd_la_LIBADD = utils/libutils.la

if ENABLETHREADING
libcwd_r_la_CXXFLAGS = -DCWDEBUG -D_REENTRANT -pthread $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
libcwd_r_la_SOURCES = \
        elf32.cc \
        environ.cc \
        bfd.cc \
        debug.cc \
        debugmalloc.cc \
        demangle.cc \
        demangle3.cc \
        strerrno.cc \
        type_info.cc \
        threading.cc \
        wrapcnclpnts.cc
libcwd_r_la_LDFLAGS = -version-info $(VERSIONINFO) $(LIB_THREADS_SHARED)
libcwd_r_la_LIBADD = utils/libutils_r.la
endif

# --------------- Maintainer's Section

dist-hook: documentation
	cp -pr $(srcdir)/documentation $(distdir)/documentation
	find $(distdir)/documentation -name CVS -print | xargs rm -rf
	find $(distdir)/documentation/tutorial \( -name 'core*' -o -name '*-bin' -o -name '*.o' \) -print | xargs rm -rf
	mkdir $(distdir)/example-project
	for i in `grep '^/' $(srcdir)/example-project/CVS/Entries | cut -d/ -f2`; do \
	  cp -p $(srcdir)/example-project/$$i $(distdir)/example-project; \
	done
	rm $(distdir)/example-project/debug.h.maintainer
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
	config.cache config.log depcomp

@MAINTAINER_MODE_TRUE@include maintMakefile
