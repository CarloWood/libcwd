## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = program
program_SOURCES = debug.cc program.cc

include_HEADERS = debug.h sys.h@MAINTAINER_MODE_FALSE@ nodebug.h
BUILT_SOURCES =@MAINTAINER_MODE_TRUE@ nodebug.h
EXTRA_DIST =@MAINTAINER_MODE_TRUE@ nodebug.h

# --------------- Maintainer's Section

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure stamp-h.in Makefile.in install-sh missing mkinstalldirs

@MAINTAINER_MODE_TRUE@include maintMakefile

