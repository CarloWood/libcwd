SUBDIRS = libcwd

BUILT_SOURCES = sys.ho sys.h
EXTRA_DIST = sys.ho.in cwd_debug.h cwd_bfd.h raw_write.h raw_write.inl ios_base_Init.h elf32.h exec_prog.h match.h private_debug_stack.inl

CXXFLAGS = @CXXFLAGS@
WARNOPTS = @WARNOPTS@
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
DEFAULT_INCLUDES =

SOURCES = pch.h.gch

MAINTAINERCLEANFILES = Makefile.in

global_dependencies=
if MAINTAINER_MODE
global_dependencies += timestamp-sys.h
endif
if BUILD_PCH
global_dependencies += timestamp-pch.h
endif

sys.h: $(global_dependencies)

@AMDEP_TRUE@ifeq (./$(DEPDIR)/pch.po, $(wildcard ./$(DEPDIR)/pch.po))
@AMDEP_TRUE@@am__include@ ./$(DEPDIR)/pch.po
@AMDEP_TRUE@endif

timestamp-pch.h pch.h.gch:
	rm -f pch.h.gch
	test -d $(DEPDIR) || mkdir $(DEPDIR)
	if $(CXX) $(DEFS) $(INCLUDES) $(CPPFLAGS) -DCWDEBUG $(WARNOPTS) $(CXXFLAGS) \
	    -M -MT timestamp-pch.h -MF $(DEPDIR)/pch.Tpo -DPIC -fPIC pch.h; then \
	  cmp -s $(DEPDIR)/pch.Tpo $(DEPDIR)/pch.po || mv $(DEPDIR)/pch.Tpo $(DEPDIR)/pch.po; \
	fi
	if $(CXX) $(DEFS) $(INCLUDES) $(CPPFLAGS) -DCWDEBUG $(WARNOPTS) $(CXXFLAGS) \
	    -DPIC -fPIC -c -o pch.h.gch pch.h; then \
	  touch timestamp-pch.h; \
	fi

clean-local:
	rm -f pch.h.gch timestamp-pch.h
