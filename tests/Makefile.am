EXTRA_PROGRAMS = \
bfd_tst \
deb \
debug_tst \
debugmalloc_tst \
flush_tst \
hello_world \
lockable_auto_ptr_tst \
magic_tst \
mao_tst \
marker \
test_delete \
type_info_tst

CLEANFILES=$(EXTRA_PROGRAMS)

bfd_tst_SOURCES = bfd_tst.cc
deb_SOURCES = deb.cc
debug_tst_SOURCES = debug_tst.cc
debugmalloc_tst_SOURCES = debugmalloc_tst.cc
flush_tst_SOURCES = flush_tst.cc
hello_world_SOURCES = hello_world.cc
lockable_auto_ptr_tst_SOURCES = lockable_auto_ptr_tst.cc
magic_tst_SOURCES = magic_tst.cc
mao_tst_SOURCES = mao_tst.cc
marker_SOURCES = marker.cc
test_delete_SOURCES = test_delete.cc
type_info_tst_SOURCES = type_info_tst.cc

LDADD = ../libcwd.la @STATICLIBS@

EXTRAOPTS = @EXTRAOPTS@
DEBUGOPTS = @DEBUGOPTS@
WARNOPTS = @WARNOPTS@
CXXFLAGS = $(DEBUGOPTS) $(EXTRAOPTS) $(WARNOPTS)
INCLUDES = -I- -I$(top_builddir)/include -I$(srcdir)/../include -DDEBUG
