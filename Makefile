# Directory where the base Makefile resides:
BASEDIR=../..

# Library name, for example: "LIB=foo" -> libfoo.so and libfoo.a
# The version of the library is read from .$(LIB).version (which is
# automatically incremented 1 Minor Version every time you (re)install.
LIB=cwd

# List of subdirectories:
SUBDIRS=utils

# List of source files:
CXXSRC:=$(shell /bin/ls -1 -t -F *$(CPPEXT) 2>/dev/null | grep '\$(CPPEXT)$$' | egrep -v '^(debug\.cc)$$') debug.cc

# Library specific include flags.  Use $(BASEDIR) as a prefix!
# For example: -I$(BASEDIR)/mylib/include
INCLUDEFLAGS=

# Use this if you include utils/perf.cc
STATICLIBS=#-L/usr/src/perf-papi/src -lpapi

# Extra static libraries to link with:
# If you defined DEBUGUSEBFD in include/libcw/debugging_defs.h,
# then you need to link with -lbfd -liberty -ldl.
# libbfd.a and liberty.a are part of GNU binutils.
SHAREDLIBS:=$(shell ./get_libraries)

#-----------------------------------------------------------------------------

include $(PROTODIR)/lib/PTMakefile

#clean::
#depend::
#build::

include .cwd.version

CWD_MAJOR:=$(VERSION_MAJOR)
CWD_MINOR:=$(VERSION_MINOR)
CWD_PATCH:=$(VERSION_PATCH)
CWD_VERSION:=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

install:
	@( if [ -f $(BASEDIR)/lib/libcwd.so.$(CWD_VERSION) ]; then \
	  make .cwd.install.timestamp; \
	else \
	  echo "libcwd.so.$(CWD_VERSION) wasn't compiled yet"; \
	fi )

.cwd.install.timestamp: $(BASEDIR)/lib/libcwd.so.$(CWD_VERSION)
	$(INSTALL) -g $(INSTALL_GROUP) -m 755 -o $(INSTALL_OWNER) $(BASEDIR)/lib/libcwd.so.$(CWD_VERSION) $(LIB_DIR)
	touch .cwd.install.timestamp
	/sbin/ldconfig
	ln -sf $(LIB_DIR)/libcwd.so.$(CWD_MAJOR) $(LIB_DIR)/libcwd.so
	$(INSTALL) -d $(PREFIX)/include/libcw
	$(INSTALL) -g $(INSTALL_GROUP) -m 644 -o $(INSTALL_OWNER) include/libcw/*.h $(PREFIX)/include/libcw
	@echo "VERSION_MAJOR=$(CWD_MAJOR)" > .cwd.version
	@echo "VERSION_MINOR=$(CWD_MINOR)" >> .cwd.version
	@echo "$(CWD_PATCH)" | awk -- '{ printf ("VERSION_PATCH=%d\n", $$0 + 1) }' >> .cwd.version
