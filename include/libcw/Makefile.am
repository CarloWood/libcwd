libcwddir = $(includedir)/libcw

libcwd_HEADERS = \
	bcd2str.h \
	bfd.h \
	buf2str.h \
	char2str.h \
	cwprint.h \
	debug.h \
	debugdebugcheckpoint.h \
	debugmalloc.h \
	demangle.h \
	exec_prog.h \
	iomanip.h \
	lockable_auto_ptr.h \
	no_alloc_checking_stringstream.h \
	strerrno.h \
	type_info.h

BUILT_SOURCES = sys.ho debug_config.ho
EXTRA_HEADERS = sys.h debug_config.h

EXTRA_DIST = perf.h

DISTCLEANFILES = debug_config.h sys.h

MAINTAINERCLEANFILES = Makefile.in

install-data-local:
	$(INSTALL_DATA) $(EXTRA_HEADERS) $(DESTDIR)$(libcwddir)

