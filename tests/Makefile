# Directory where the base Makefile resides:
BASEDIR=../../..

# List of source files (one per executable):
CXXSRC:=$(shell grep -l 'main' *.cc)

# List of sub directories:
SUBDIRS=

# Extra static libraries to link with:
STATICLIBS=

# Extra shared libraries to link with:
SHAREDLIBS=-lcwd $(shell ../get_libraries)

# Use -L$(LIBCWDIR)/lib to use the latest alpha version:
LIBFLAGS=

# Extra *.o files that need to be linked:
EXTRA_OBJS=

#-----------------------------------------------------------------------------

include $(PROTODIR)/test/PTMakefile

#build::
#clean::
#depend::
