# The directory where the base Makefile resides:
BASEDIR=../../..

# List of subdirectories:
SUBDIRS=

# List source files:
CXXSRC:=$(shell /bin/ls -1 -t -F *$(CPPEXT) 2>/dev/null | grep '\$(CPPEXT)$$' | egrep -v '^(perf\.cc)$$')

#-----------------------------------------------------------------------------

include $(PROTODIR)/sub/PTMakefile

#build::
#clean::
#depend::
