cmake_minimum_required(VERSION 3.9...3.16)

project(libcwd
        VERSION 1.1.1
        DESCRIPTION "Libcwd is a thread-safe, full-featured debugging support library for C++ developers."
        HOMEPAGE_URL "http://carlowood.github.io/libcwd/"
        LANGUAGES CXX C
)

include(cwm4/cmake/AICxxProject)

cw_option(EnableGlibCxxDEBUG
          "compile libcwd with -D_GLIBCXX_DEBUG. See http://gcc.gnu.org/onlinedocs/libstdc++/debug.html" OFF
          "" OFF
)
cw_option(EnableLibcwdAlloc
          "Enable memory allocation debugging support" ON
          "" OFF
)
cw_option(EnableLibcwdMagic
          "Enable magic numbers in memory allocation debugging" ON
          "" OFF
)
cw_option(EnableLibcwdMarker
          "Enable memory allocation markers" ON
          "" OFF
)
cw_option(EnableLibcwdLocation
          "Enable the ability to lookup source file and line number, you might need this on a non-ELF system" ON
          "" OFF
)
cw_option(EnableLibcwdDebug
          "Enable debugging of libcwd itself" OFF
          "" OFF
)
cw_option(EnableLibcwdDebugOutput
          "Enable debug output of EnableLibcwdDebug" ON
          "OptionEnableLibcwdDebug" OFF
)
cw_option(EnableLibcwdDebugm
          "Enable debugging of memory allocation debugging code in libcwd itself" OFF
          "OptionEnableLibcwdDebug" OFF
)
cw_option(EnableLibcwdDebugt
          "Enable debugging of threading debugging code in libcwd itself" OFF
          "OptionEnableLibcwdDebug" OFF
)

if (OptionEnableGlibCxxDEBUG)
  set(CW_CONFIG_GLIBCXX_DEBUG 1)
else ()
  set(CW_CONFIG_GLIBCXX_DEBUG 0)
endif ()
if (OptionEnableLibcwdAlloc)
  set(CW_CONFIG_ALLOC 1)
else ()
  set(CW_CONFIG_ALLOC 0)
endif ()
if (OptionEnableLibcwdMagic)
  set(CW_CONFIG_MAGIC 1)
else ()
  set(CW_CONFIG_MAGIC 0)
endif ()
if (OptionEnableLibcwdMarker)
  set(CW_CONFIG_MARKER 1)
else ()
  set(CW_CONFIG_MARKER 0)
endif ()
if (OptionEnableLibcwdLocation)
  set(CW_CONFIG_LOCATION 1)
else ()
  set(CW_CONFIG_LOCATION 0)
endif ()
if (OptionEnableLibcwdDebug)
  set(CW_CONFIG_DEBUG 1)
else ()
  set(CW_CONFIG_DEBUG 0)
endif ()
if (OptionEnableLibcwdDebugOutput)
  set(CW_CONFIG_DEBUGOUTPUT 1)
else ()
  set(CW_CONFIG_DEBUGOUTPUT 0)
endif ()
if (OptionEnableLibcwdDebugm)
  set(CW_CONFIG_DEBUGM 1)
else ()
  set(CW_CONFIG_DEBUGM 0)
endif ()
if (OptionEnableLibcwdDebugt)
  set(CW_CONFIG_DEBUGT 1)
else ()
  set(CW_CONFIG_DEBUGT 0)
endif ()
# On non-ELF systems this should be a real option, I guess.
set(CW_CONFIG_LIBBFD 0)
# Needed for include/libcwd/config.h.in
set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_REVISION ${PROJECT_VERSION_PATCH})

include(CW_DEFINE_TYPE)
include(CheckPrototypeDefinition)

include( CheckCXXSourceCompiles )
check_cxx_source_compiles( "int main(void) { return __builtin_expect(0, 1); }" HAVE_BUILTIN_EXPECT )

include( CW_SYS_MALLOC_OVERHEAD )
CW_SYS_MALLOC_OVERHEAD()                # Determine the overhead in bytes of an allocation with malloc(2).

include( CheckIncludeFileCXX )
check_include_file_cxx("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file_cxx("pwd.h" HAVE_PWD_H)
check_include_file_cxx("malloc.h" HAVE_MALLOC_H)
check_include_file_cxx("unistd.h" HAVE_UNISTD_H)
check_include_file_cxx("pthread.h" HAVE_PTHREAD_H)
if (HAVE_PTHREAD_H)
  set(CW_CONFIG_HAVE_PTHREADS define)
else ()
  set(CW_CONFIG_HAVE_PTHREADS undef)
endif ()

include( CheckFunctionExists )
if (CMAKE_DL_LIBS)
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
endif ()
check_function_exists(dlopen HAVE_DLOPEN)
check_function_exists(_dl_loaded HAVE__DL_LOADED)
check_function_exists(_rtld_global HAVE__RTLD_GLOBAL)
unset(CMAKE_REQUIRED_LIBRARIES)

if (HAVE_DLOPEN)
  set(CW_CONFIG_HAVE_DLOPEN define)
else ()
  set(CW_CONFIG_HAVE_DLOPEN undef)
endif()

check_function_exists(__libc_malloc HAVE___LIBC_MALLOC)
check_function_exists(_libc_malloc HAVE__LIBC_MALLOC)
check_function_exists(aligned_alloc HAVE_ALIGNED_ALLOC)
check_function_exists(memalign HAVE_MEMALIGN)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(valloc HAVE_VALLOC)
check_function_exists(wmemcpy HAVE_WMEMCPY)
check_function_exists(getegid HAVE_GETEGID)
check_function_exists(getgid HAVE_GETGID)
check_function_exists(getgroups HAVE_GETGROUPS)

if (HAVE_GETGROUPS)
  check_prototype_definition(getgroups "int getgroups(int size, gid_t list[])" "NULL" "unistd.h" HAVE_GET_T)
  if (HAVE_GET_T)
    CW_DEFINE_TYPE(gid_t getgroups_t)
  else()
    CW_DEFINE_TYPE(int getgroups_t)
  endif()
endif()

if (HAVE___LIBC_MALLOC)
  set(CW_CONFIG_HAVE___LIBC_MALLOC define)
else ()
  set(CW_CONFIG_HAVE___LIBC_MALLOC undef)
endif ()
if (HAVE__LIBC_MALLOC)
  set(CW_CONFIG_HAVE__LIBC_MALLOC define)
else ()
  set(CW_CONFIG_HAVE__LIBC_MALLOC undef)
endif ()
if (HAVE_ALIGNED_ALLOC)
  set(CW_CONFIG_HAVE_ALIGNED_ALLOC define)
else ()
  set(CW_CONFIG_HAVE_ALIGNED_ALLOC undef)
endif ()
if (HAVE_MEMALIGN)
  set(CW_CONFIG_HAVE_MEMALIGN define)
else ()
  set(CW_CONFIG_HAVE_MEMALIGN undef)
endif()
if (HAVE_POSIX_MEMALIGN)
  set(CW_CONFIG_HAVE_POSIX_MEMALIGN define)
else ()
  set(CW_CONFIG_HAVE_POSIX_MEMALIGN undef)
endif()
if (HAVE_VALLOC)
  set(CW_CONFIG_HAVE_VALLOC define)
else ()
  set(CW_CONFIG_HAVE_VALLOC undef)
endif()

# Set a decent default value in case we don't have the right libraries.
if (CMAKE_SYSTEM_PROCESSOR MATCHES "sparc")
  set(CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET 0)
else ()
  set(CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET -1)
endif ()

if (NOT THREADSMAX)
  set(THREADSMAX 1024)
endif ()

find_program(CW_PATH_PROG_PS "ps" DOC "Full path to the 'ps' executable.")
# The PS_WIDE_PID_OPTION test is not needed when EnableLibcwdLocation is OFF.
include(CW_SYS_PS_WIDE_PID_OPTION)
if (OptionEnableLibcwdLocation)
  CW_SYS_PS_WIDE_PID_OPTION()
endif ()

configure_file("cmake-config.h.in" "config.h")
configure_file("include/libcwd/config.h.in" "include/libcwd/config.h" @ONLY)
configure_file("include/libcwd/sys.h.in" "include/libcwd/sys.h" @ONLY)
configure_file("include/sys.h.in" "include/sys.h" @ONLY)

find_package( Threads REQUIRED )

add_library(cwd_r SHARED)

set_target_properties(cwd_r PROPERTIES VERSION 8.0.3)
set_target_properties(cwd_r PROPERTIES SOVERSION 8)

target_sources(cwd_r
    PRIVATE
        alloc_filter.cc
        bfd.cc
        compilation_unit.cc
        debug.cc
        debugmalloc.cc
        demangle3.cc
        elfxx.cc
        environ.cc
        function.cc
        private_allocator.cc
        strerrno.cc
        threading.cc
        type_info.cc
        wrapcnclpnts.cc
)

set_target_properties(cwd_r PROPERTIES
    PUBLIC_HEADER
        include/libcwd/attach_gdb.h
	include/libcwd/bfd.h
	include/libcwd/buf2str.h
	include/libcwd/char2str.h
	include/libcwd/class_alloc_filter.h
	include/libcwd/class_alloc.h
	include/libcwd/class_always_channel.h
	include/libcwd/class_channel.h
	include/libcwd/class_channel_set.h
	include/libcwd/class_continued_channel.h
	include/libcwd/class_debug.h
	include/libcwd/class_debug_string.h
	include/libcwd/class_fatal_channel.h
	include/libcwd/class_function.h
	include/libcwd/class_location.h
	include/libcwd/class_marker.h
	include/libcwd/class_object_file.h
	include/libcwd/class_rcfile.h
	include/libcwd/control_flag.h
	include/libcwd/core_dump.h
	include/libcwd/cwlist.h
	include/libcwd/cwprint.h
	include/libcwd/debug.h
	include/libcwd/debugmalloc.h
	include/libcwd/demangle.h
	include/libcwd/enum_memblk_types.h
	include/libcwd/libraries_debug.h
	include/libcwd/lockable_auto_ptr.h
	include/libcwd/macro_AllocTag.h
	include/libcwd/macro_ForAllDebugChannels.h
	include/libcwd/macro_ForAllDebugObjects.h
	include/libcwd/macro_Libcwd_macros.h
	include/libcwd/max_label_len.h
	include/libcwd/pc_mangled_function_name.h
	include/libcwd/private_allocator.h
	include/libcwd/private_assert.h
	include/libcwd/private_bufferstream.h
	include/libcwd/private_debug_stack.h
	include/libcwd/private_environ.h
	include/libcwd/private_internal_stringbuf.h
	include/libcwd/private_internal_string.h
	include/libcwd/private_internal_stringstream.h
	include/libcwd/private_internal_vector.h
	include/libcwd/private_lock_interface.h
	include/libcwd/private_mutex.h
	include/libcwd/private_mutex_instances.h
	include/libcwd/private_set_alloc_checking.h
	include/libcwd/private_string.h
	include/libcwd/private_struct_TSD.h
	include/libcwd/private_thread.h
	include/libcwd/private_threading.h
	include/libcwd/smart_ptr.h
	include/libcwd/strerrno.h
	include/libcwd/struct_debug_tsd.h
	include/libcwd/type_info.h
)

target_compile_definitions(cwd_r
    PUBLIC
        CWDEBUG
        LIBCWD_THREAD_SAFE
        VERSION="${PROJECT_VERSION}"
)

target_include_directories(cwd_r
    PRIVATE
        ${top_objdir}/include
        ${top_srcdir}/include
)

target_link_libraries( cwd_r
    PUBLIC
        Threads::Threads
)

include(GNUInstallDirs)

install(TARGETS cwd_r
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#add_subdirectory
