noinst_HEADERS = sys.h

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir)/include/libcwd
LDADD = ../libcwd.la

test_files = t.basic.h

t_runners = $(test_files:.h=.cutee.cc)

all-local:

check: runtest
	./runtest

clean-local: runtest-clean
	rm -f tests.out

noinst_PROGRAMS = cutee

cutee_SOURCES = cutee.cc cutee.h
cutee_LDADD =

EXTRA_PROGRAMS = runtest
runtest_SOURCES = runtest.cc $(test_files) $(t_runners)
runtest_DEPENDENCIES = cutee
runtest_CXXFLAGS = -DCWDEBUG -include sys.h

%.cutee.cc: $(srcdir)/%.h
	./cutee -o $@ $<

runtest.cc: cutee
	./cutee --main -o $@

runtest-clean:
	rm -f cutee *.o *.cutee.cc runtest runtest.cc

dist-hook:
	rm -f $(distdir)/runtest.cc $(distdir)/$(t_runners)

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
