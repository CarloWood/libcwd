libcwddir = $(includedir)/libcw

libcwd_HEADERS = \
	bfd.h \
	buf2str.h \
	char2str.h \
	class_alloc.h \
	class_always_channel.h \
	class_always_channel.inl \
	class_channel.h \
	class_channel.inl \
	class_channel_set.h \
	class_channel_set.inl \
	class_continued_channel.h \
	class_continued_channel.inl \
	class_debug.h \
	class_debug.inl \
	class_debug_string.h \
	class_debug_string.inl \
	class_fatal_channel.h \
	class_fatal_channel.inl \
	class_location.h \
	class_location.inl \
	class_marker.h \
	control_flag.h \
	core_dump.h \
	cwprint.h \
	debug.h \
	debugmalloc.h \
	demangle.h \
	enum_memblk_types.h \
	lockable_auto_ptr.h \
	macro_AllocTag.h \
	macro_ForAllDebugChannels.h \
	macro_ForAllDebugObjects.h \
	macro_Libcwd_macros.h \
	max_label_len.h \
	pc_mangled_function_name.h \
	private_TSD.h \
	private_allocator.h \
	private_assert.h \
	private_debug_stack.h \
	private_internal_string.h \
	private_internal_stringstream.h \
	private_internal_vector.h \
	private_set_alloc_checking.h \
	private_threading.h \
	strerrno.h \
	struct_debug_tsd.h \
	type_info.h

BUILT_SOURCES = sysd.ho debug_config.ho
EXTRA_HEADERS = sysd.h debug_config.h

DISTCLEANFILES = debug_config.h sysd.h

MAINTAINERCLEANFILES = Makefile.in

install-data-local:
	$(INSTALL_DATA) $(EXTRA_HEADERS) $(DESTDIR)$(libcwddir)

