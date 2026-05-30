cmake_minimum_required(VERSION 3.9...3.30)

# Compile all subdirectories with the same standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

project(libcwd
        VERSION 2.0.0
        DESCRIPTION "Libcwd is a thread-safe, full-featured debugging support library for C++ developers."
        HOMEPAGE_URL "http://carlowood.github.io/libcwd/"
        LANGUAGES CXX C
)

include(CTest)

# Version info, see http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
#
# CW_INTERFACE_VERSION           : Current interface version, increment whenever the interface is changed.
# CW_INTERFACE_VERSION_REVISION  : Revision, increment every public release; or set to 0 when the interface is changed.
# CW_INTERFACE_AGE               : Increment if interfaces have been added; or set to 0 when interfaces have been removed.

set(CW_INTERFACE_VERSION 11)
set(CW_INTERFACE_VERSION_REVISION 0)
set(CW_INTERFACE_AGE 0)

#==============================================================================
# This project uses aicxx modules.
include(cmake/aicxx/Project NO_POLICY_SCOPE)

#==============================================================================
# OPTIONS

# Exit if someone tries to contaminate the source directory with an in-source build.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Please do out-of-source builds."
                      "Cleanup: \"rm -rf CMake CMakeCache.txt CMakeFiles/\"")
endif ()

cw_option(EnableGlibCxxDebug
          "compile libcwd with -D_GLIBCXX_DEBUG. See http://gcc.gnu.org/onlinedocs/libstdc++/debug.html" OFF
          "" OFF
)
cw_option(EnableLibcwdLocation
          "Enable the ability to lookup source file and line number. Turn OFF on for example non-ELF systems" ON
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
cw_option(EnableLibcwdDebugt
          "Enable debugging of threading debugging code in libcwd itself" OFF
          "OptionEnableLibcwdDebug" OFF
)

if (OptionEnableGlibCxxDebug)
  set(CW_CONFIG_GLIBCXX_DEBUG 1)
else ()
  set(CW_CONFIG_GLIBCXX_DEBUG 0)
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

include(CheckIncludeFileCXX)
check_include_file_cxx("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file_cxx("pwd.h" HAVE_PWD_H)
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

check_function_exists(getegid HAVE_GETEGID)
check_function_exists(getgid HAVE_GETGID)
check_function_exists(getgroups HAVE_GETGROUPS)

if (HAVE_GETGROUPS)
  CW_DEFINE_TYPE(gid_t getgroups_t)
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

# Needed for subst-config.h.in, then used in subdirectory utils.
set(libcwd_datadir "${CMAKE_INSTALL_PREFIX}/share")

configure_file("cmake-config.h.in" "config.h")
configure_file("include/libcwd/config.h.in" "include/libcwd/config.h" @ONLY)
configure_file("include/libcwd/sys.h.in" "include/libcwd/sys.h" @ONLY)
configure_file("include/cwd_sys.h.in" "include/cwd_sys.h" @ONLY)

# Clean up rule for the above.
add_custom_target(libcwd-maintainer-clean-extra
    COMMAND rm config.h include/cwd_sys.h include/libcwd/config.h include/libcwd/sys.h
    COMMAND rm -f libcwdConfig.cmake libcwdConfigVersion.cmake libcwd.pc
)

set(cwd_compile_definitions "CWDEBUG")
if (OptionEnableGlibCxxDebug)
  list(APPEND cwd_compile_definitions "_GLIBCXX_DEBUG")
endif ()

find_package(Threads REQUIRED)
find_library(LIBDW dw REQUIRED)
find_library(LIBELF elf REQUIRED)

add_subdirectory(utils)

add_library(cwd SHARED)

math(EXPR INTERFACE_cwd_MAJOR_VERSION "${CW_INTERFACE_VERSION} - ${CW_INTERFACE_AGE}" OUTPUT_FORMAT DECIMAL)
set(INTERFACE_cwd_MINOR_VERSION ${CW_INTERFACE_AGE})
set(INTERFACE_cwd_PATCH_VERSION ${CW_INTERFACE_VERSION_REVISION})
set_target_properties(cwd PROPERTIES VERSION ${INTERFACE_cwd_MAJOR_VERSION}.${INTERFACE_cwd_MINOR_VERSION}.${INTERFACE_cwd_PATCH_VERSION})
set_target_properties(cwd PROPERTIES SOVERSION ${INTERFACE_cwd_MAJOR_VERSION})
# Note really sure what I'm doing here:
set_property(TARGET cwd PROPERTY INTERFACE_cwd_MAJOR_VERSION ${INTERFACE_cwd_MAJOR_VERSION})
set_property(TARGET cwd APPEND PROPERTY COMPATIBLE_INTERFACE_STRING cwd_MAJOR_VERSION)

target_sources(cwd
    PRIVATE
        debug.cc
        demangle3.cc
        dwarf.cc
        environ.cc
        location.cc
        strerrno.cc
        threading.cc
        type_info.cc
        wrapcnclpnts.cc
)

file(GLOB PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "include/libcwd/*.h")
file(GLOB PUBLIC_HEADERS_INL RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "include/libcwd/*.inl")
list(APPEND PUBLIC_HEADERS "${PUBLIC_HEADERS_INL}")
list(APPEND PUBLIC_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/include/libcwd/sys.h" "${CMAKE_CURRENT_BINARY_DIR}/include/libcwd/config.h")
set_target_properties(cwd PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

target_compile_definitions(cwd
    PUBLIC
        "${cwd_compile_definitions}"
        VERSION="${PROJECT_VERSION}"
)

target_include_directories(cwd
    PRIVATE
      "${CMAKE_BINARY_DIR}/include"
      "${CMAKE_SOURCE_DIR}/include"
      "${CMAKE_SOURCE_DIR}"
    INTERFACE
        "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

target_link_libraries(cwd
    PRIVATE
        Libcwd::utils
    PUBLIC
        Threads::Threads
    INTERFACE
        "${CMAKE_DL_LIBS}"
        "${LIBDW}"
        "${LIBELF}"
)

include(GNUInstallDirs)

install(TARGETS cwd
  EXPORT libcwdTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libcwd
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/libcwdConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/libcwdConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/libcwdConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/libcwd)
install(EXPORT libcwdTargets
  FILE
    libcwdTargets.cmake
  NAMESPACE
    Libcwd::
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libcwdConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libcwdConfigVersion.cmake"
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
string (REPLACE ";" " -D" _TMP_STR "${cwd_compile_definitions}")
set(PCCFLAGS "-D${_TMP_STR}")
configure_file(${libcwd_SOURCE_DIR}/libcwd.pc.in ${libcwd_BINARY_DIR}/libcwd.pc @ONLY)
install(
  FILES
    ${CMAKE_BINARY_DIR}/libcwd.pc
  DESTINATION
    lib${LIB_SUFFIX}/pkgconfig
)

add_subdirectory(include/threadsafe)

target_include_directories(threadsafe_ObjLib
  PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/include"
)
target_compile_definitions(threadsafe_ObjLib
  PRIVATE
    ${cwd_compile_definitions}
    "-DCW_DEBUG=1"
)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif ()
