## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = program

include_HEADERS = debug.h sys.h
program_SOURCES = debug.cc program.cc

# --------------- Maintainer's Section

distclean-local:
	rm -f config.cache config.log config.status

MAINTAINERCLEANFILES = $(srcdir)/aclocal.m4 $(srcdir)/config.h.in $(srcdir)/configure stamp-h.in $(srcdir)/Makefile.in \
        $(srcdir)/install-sh $(srcdir)/missing $(srcdir)/mkinstalldirs

