SUBDIRS = libcwd

BUILT_SOURCES = sys.h
if BUILD_PCH
if ENABLENONTHREADING
BUILT_SOURCES += pch.h.gch/libcwd_pch.h
endif
if ENABLETHREADING
BUILT_SOURCES += pch.h.gch/libcwd_r_pch.h
endif
endif
EXTRA_DIST = sys.h.in cwd_debug.h cwd_bfd.h raw_write.h raw_write.inl ios_base_Init.h elf32.h exec_prog.h \
             match.h private_debug_stack.inl compilation_unit.h demangle.h pch-source.h

CXXFLAGS = @CXXFLAGS@
WARNOPTS = @WARNOPTS@
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
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

pch.h.gch/libcwd_pch.h pch.h.gch/libcwd_r_pch.h: $(srcdir)/pch-source.h Makefile
	test -d pch.h.gch || mkdir -p pch.h.gch
	rm -f $@
	test -d $(DEPDIR) || mkdir $(DEPDIR)
if ENABLENONTHREADING
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG $(WARNOPTS) $(EXTRAOPTS) $(CXXFLAGS) -DPIC -fPIC \
	    -M -MT pch.h.gch/libcwd_pch.h -MF $(DEPDIR)/libcwd_pch.Tpo -DPIC -fPIC $(srcdir)/pch-source.h >/dev/null; then \
	  cmp -s $(DEPDIR)/libcwd_pch.Tpo $(DEPDIR)/libcwd_pch.po || mv $(DEPDIR)/libcwd_pch.Tpo $(DEPDIR)/libcwd_pch.po; \
	fi
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG $(WARNOPTS) $(EXTRAOPTS) $(CXXFLAGS) -DPIC -fPIC \
	    -c -o pch.h.gch/libcwd_pch.h $(srcdir)/pch-source.h
endif
if ENABLETHREADING
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG -D_REENTRANT -pthread $(WARNOPTS) $(EXTRAOPTS) $(CXXFLAGS) -DPIC -fPIC \
	    -M -MT pch.h.gch/libcwd_r_pch.h -MF $(DEPDIR)/libcwd_r_pch.Tpo -DPIC -fPIC $(srcdir)/pch-source.h >/dev/null; then \
	  cmp -s $(DEPDIR)/libcwd_r_pch.Tpo $(DEPDIR)/libcwd_r_pch.po || mv $(DEPDIR)/libcwd_r_pch.Tpo $(DEPDIR)/libcwd_r_pch.po; \
	fi
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -DCWDEBUG -D_REENTRANT -pthread $(WARNOPTS) $(EXTRAOPTS) $(CXXFLAGS) -DPIC -fPIC \
	    -c -o pch.h.gch/libcwd_r_pch.h $(srcdir)/pch-source.h
endif

clean-local:
	rm -f pch.h.gch/* $(DEPDIR)/libcwd_*.*po

distclean-local:
	rm -rf pch.h.gch .deps

