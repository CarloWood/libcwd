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
	h.h \
	iomanip.h \
	lockable_auto_ptr.h \
	no_alloc_checking_ostrstream.h \
	strerrno.h \
	type_info.h

EXTRA_DIST = perf.h

debug_config.h: debug_config.ho
	@(if cmp -s debug_config.h debug_config.ho 2> /dev/null; then \
	    echo "debug_config.h is unchanged"; \
	    touch -r debug_config.h debug_config.ho; \
	  else \
	    cp debug_config.ho debug_config.h; \
	  fi)

sys.h: sys.ho
	@(if cmp -s sys.h sys.ho 2> /dev/null; then \
	    echo "sys.h is unchanged"; \
	    touch -r sys.h sys.ho; \
	  else \
	    cp sys.ho sys.h; \
	  fi)

DISTCLEANFILES = debug_config.h sys.h

