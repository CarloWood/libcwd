libcwddir = $(includedir)/libcwd

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
	class_function.h \
	class_location.h \
	class_location.inl \
	class_marker.h \
	class_object_file.h \
	class_ooam_filter.h \
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
	private_allocator.h \
	private_allocator.inl \
	private_assert.h \
	private_bufferstream.h \
	private_debug_stack.h \
	private_environ.h \
	private_internal_string.h \
	private_internal_stringbuf.h \
	private_internal_stringstream.h \
	private_internal_vector.h \
	private_lock_interface.h \
	private_mutex.h \
	private_mutex.inl \
	private_mutex_instances.h \
	private_set_alloc_checking.h \
	private_string.h \
	private_struct_TSD.h \
	private_thread.h \
	private_threading.h \
	set_ostream.inl \
	smart_ptr.h \
	strerrno.h \
	struct_debug_tsd.h \
	type_info.h

BUILT_SOURCES = sys.ho config.ho sys.h config.h
EXTRA_HEADERS = sys.h config.h

MAINTAINERCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(libcwddir)
	$(INSTALL_DATA) $(EXTRA_HEADERS) $(DESTDIR)$(libcwddir)

uninstall-local:
	cd $(DESTDIR)$(libcwddir); rm -f $(EXTRA_HEADERS)
	rmdir $(DESTDIR)$(libcwddir)

@MAINTAINER_MODE_TRUE@sys.h: timestamp-sys.h
@MAINTAINER_MODE_TRUE@config.h: timestamp-config.h
