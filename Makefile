# Directory where the base Makefile resides:
BASEDIR=../..

# Library name, for example: "LIB=foo" -> libfoo.so and libfoo.a
# The version of the library is read from .$(LIB).version (which is
# automatically incremented 1 Minor Version every time you (re)install.
LIB=cwd

# List of subdirectories:
SUBDIRS=utils

# List of source files:
CXXSRC:=$(shell find . -maxdepth 1 -mindepth 1 -type f -name '*.cc' ! -name debug.cc -printf '%f\n') debug.cc

# Library specific include flags.  Use $(BASEDIR) as a prefix!
# For example: -I$(BASEDIR)/mylib/include
INCLUDEFLAGS=

# Use this if you include utils/perf.cc
STATICLIBS=#-L/usr/src/perf-papi/src -lpapi

# Extra static libraries to link with:
# If you defined DEBUGUSEBFD in include/libcw/debugging_defs.h,
# then you need this line; libbfd.a is part of GNU binutils.
SHAREDLIBS=-lbfd -liberty -ldl

#-----------------------------------------------------------------------------

include $(PROTODIR)/lib/PTMakefile

#clean::
#depend::
#build::
