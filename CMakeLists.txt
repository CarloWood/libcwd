cmake_minimum_required(VERSION 3.9...3.16)

project(libcwd
        VERSION 1.1.2
        DESCRIPTION "Libcwd is a thread-safe, full-featured debugging support library for C++ developers."
        HOMEPAGE_URL "http://carlowood.github.io/libcwd/"
        LANGUAGES CXX C
)

# Version info, see http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
#
# CW_INTERFACE_VERSION           : Current interface version, increment whenever the interface is changed.
# CW_INTERFACE_VERSION_REVISION  : Revision, increment every public release; or set to 0 when the interface is changed.
# CW_INTERFACE_AGE               : Increment if interfaces have been added; or set to 0 when interfaces have been removed.

set(CW_INTERFACE_VERSION 8)
set(CW_INTERFACE_VERSION_REVISION 1)
set(CW_INTERFACE_AGE 3)

include(cwm4/cmake/AICxxProject)

cw_option(EnableGlibCxxDebug
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

if (OptionEnableGlibCxxDebug)
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

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("int main(void) { return __builtin_expect(0, 1); }" HAVE_BUILTIN_EXPECT)

include(CW_SYS_MALLOC_OVERHEAD)
CW_SYS_MALLOC_OVERHEAD()                # Determine the overhead in bytes of an allocation with malloc(2).

include(CheckIncludeFileCXX)
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

include(CheckFunctionExists)
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

# Needed for cmake-config.h.in, then used in subdirectory utils.
set(libcwd_datadir "${CMAKE_INSTALL_PREFIX}/share")

configure_file("cmake-config.h.in" "config.h")
configure_file("include/libcwd/config.h.in" "include/libcwd/config.h" @ONLY)
configure_file("include/libcwd/sys.h.in" "include/libcwd/sys.h" @ONLY)
configure_file("include/sys.h.in" "include/sys.h" @ONLY)

# Clean up rule for the above.
add_custom_target(maintainer-clean-extra
    COMMAND rm config.h include/sys.h include/libcwd/config.h include/libcwd/sys.h
    COMMAND rm -rf cw_utils_sys_malloc_overhead
)

set(cwd_r_compile_definintions "CWDEBUG;LIBCWD_THREAD_SAFE")
if (OptionEnableGlibCxxDebug)
  list(APPEND cwd_r_compile_definintions "_GLIBCXX_DEBUG")
endif ()

find_package(Threads REQUIRED)

add_subdirectory(utils)

add_library(cwd_r SHARED)

math(EXPR INTERFACE_cwd_r_MAJOR_VERSION "${CW_INTERFACE_VERSION} - ${CW_INTERFACE_AGE}" OUTPUT_FORMAT DECIMAL)
set(INTERFACE_cwd_r_MINOR_VERSION ${CW_INTERFACE_AGE})
set(INTERFACE_cwd_r_PATCH_VERSION ${CW_INTERFACE_VERSION_REVISION})
set_target_properties(cwd_r PROPERTIES VERSION ${INTERFACE_cwd_r_MAJOR_VERSION}.${INTERFACE_cwd_r_MINOR_VERSION}.${INTERFACE_cwd_r_PATCH_VERSION})
set_target_properties(cwd_r PROPERTIES SOVERSION ${INTERFACE_cwd_r_MAJOR_VERSION})
# Note really sure what I'm doing here:
set_property(TARGET cwd_r PROPERTY INTERFACE_cwd_r_MAJOR_VERSION ${INTERFACE_cwd_r_MAJOR_VERSION})
set_property(TARGET cwd_r APPEND PROPERTY COMPATIBLE_INTERFACE_STRING cwd_r_MAJOR_VERSION)

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

file(GLOB PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "include/libcwd/*.h")
set_target_properties(cwd_r PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

target_compile_definitions(cwd_r
    PUBLIC
        "${cwd_r_compile_definintions}"
        VERSION="${PROJECT_VERSION}"
)

target_include_directories(cwd_r
    PRIVATE
        ${top_objdir}/include
        ${top_srcdir}/include
    INTERFACE
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>
)

target_link_libraries(cwd_r
    PRIVATE
        Libcwd::utils
    PUBLIC
        Threads::Threads
    INTERFACE
        "${CMAKE_DL_LIBS}"
)

include(GNUInstallDirs)

install(TARGETS cwd_r
  EXPORT libcwd_rTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libcwd
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/libcwd_rConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/libcwd_rConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/cwd_rConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/libcwd_r)
install(EXPORT libcwd_rTargets
  FILE
    libcwd_rTargets.cmake
  NAMESPACE
    Libcwd::
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
    cmake/libcwd_rConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/libcwd_rConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)
install(
  FILES
    libcwdrc
  DESTINATION
    "${libcwd_datadir}/libcwd"
)

# PkgConfig
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(includedir  \${prefix}/include)
if (${UNIX})
  set(libdir    \${exec_prefix}/lib)
endif ()
if (${WIN32})
  set(libdir    \${exec_prefix}/bin)
endif ()
string (REPLACE ";" " -D" _TMP_STR "${cwd_r_compile_definintions}")
set(PCCFLAGSR "-D${_TMP_STR}")
configure_file(${CMAKE_SOURCE_DIR}/libcwd_r.pc.in ${CMAKE_BINARY_DIR}/libcwd_r.pc @ONLY)
install(
  FILES
    ${CMAKE_BINARY_DIR}/libcwd_r.pc
  DESTINATION
    lib${LIB_SUFFIX}/pkgconfig
)
