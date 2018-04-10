AM_CPPFLAGS = -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

bin_PROGRAMS = helloworld

helloworld_SOURCES = \
	helloworld.cpp \
	debug.cpp \
	debug.h

helloworld_CXXFLAGS = @LIBCWD_R_FLAGS@
helloworld_LDADD = ../cwds/libcwds_r.la

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
