SUBDIRS = libcwd

BUILT_SOURCES = sys.h
EXTRA_DIST = sys.h.in cwd_debug.h cwd_bfd.h raw_write.h raw_write.inl ios_base_Init.h elfxx.h exec_prog.h \
             match.h zone.h private_debug_stack.inl compilation_unit.h demangle.h environment.h
TAGS_FILES = $(EXTRA_DIST)

CXXFLAGS = @CXXFLAGS@
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include
DEFAULT_INCLUDES =

DISTCLEANFILES = $(BUILT_SOURCES)
MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

global_dependencies=
if MAINTAINER_MODE
global_dependencies += timestamp-sys.h
endif

sys.h: $(global_dependencies)

clean-local:
	rm -f $(DEPDIR)/libcwd_*.*po

distclean-local:
	rm -rf .deps
