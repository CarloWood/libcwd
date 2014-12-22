SUBDIRS = libcwd

BUILT_SOURCES = sys.h
if USE_PCH
if ENABLENONTHREADING
BUILT_SOURCES += pch.h.gch/libcwd_pch.h
endif
if ENABLETHREADING
BUILT_SOURCES += pch.h.gch/libcwd_r_pch.h
endif
endif
EXTRA_DIST = sys.h.in cwd_debug.h cwd_bfd.h raw_write.h raw_write.inl ios_base_Init.h elfxx.h exec_prog.h \
             match.h zone.h private_debug_stack.inl compilation_unit.h demangle.h environment.h pch-source.h
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

if ENABLENONTHREADING
@true@ifeq (./$(DEPDIR)/libcwd_pch.po, $(wildcard ./$(DEPDIR)/libcwd_pch.po))
@true@include ./$(DEPDIR)/libcwd_pch.po
@true@endif
endif
if ENABLETHREADING
@true@ifeq (./$(DEPDIR)/libcwd_r_pch.po, $(wildcard ./$(DEPDIR)/libcwd_r_pch.po))
@true@include ./$(DEPDIR)/libcwd_r_pch.po
@true@endif
endif

pch.h.gch:
	mkdir pch.h.gch

pch.h.gch/libcwd_pch.h: pch.h.gch $(srcdir)/pch-source.h Makefile
	rm -f $@
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG $(CXXFLAGS) -DPIC -fPIC \
	    -M -MT $@ -MF $(DEPDIR)/libcwd_pch.Tpo -DPIC -fPIC $(srcdir)/pch-source.h >/dev/null; then \
	  cmp -s $(DEPDIR)/libcwd_pch.Tpo $(DEPDIR)/libcwd_pch.po || mv $(DEPDIR)/libcwd_pch.Tpo $(DEPDIR)/libcwd_pch.po; \
	fi
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG $(CXXFLAGS) -DPIC -fPIC -c -o $@ $(srcdir)/pch-source.h

pch.h.gch/libcwd_r_pch.h: pch.h.gch $(srcdir)/pch-source.h Makefile
	rm -f $@
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG -DLIBCWD_THREAD_SAFE -D_REENTRANT -pthread $(CXXFLAGS) -DPIC -fPIC \
	    -M -MT $@ -MF $(DEPDIR)/libcwd_r_pch.Tpo -DPIC -fPIC $(srcdir)/pch-source.h >/dev/null; then \
	  cmp -s $(DEPDIR)/libcwd_r_pch.Tpo $(DEPDIR)/libcwd_r_pch.po || mv $(DEPDIR)/libcwd_r_pch.Tpo $(DEPDIR)/libcwd_r_pch.po; \
	fi
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG -DLIBCWD_THREAD_SAFE -D_REENTRANT -pthread $(CXXFLAGS) -DPIC -fPIC -c -o $@ $(srcdir)/pch-source.h

clean-local:
	rm -f pch.h.gch/* $(DEPDIR)/libcwd_*.*po

distclean-local:
	rm -rf pch.h.gch .deps

