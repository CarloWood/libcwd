cmake_minimum_required(VERSION 3.18)

project(cwds
  LANGUAGES CXX
  DESCRIPTION "Application-side libcwd support code"
)

include(AICxxProject OPTIONAL RESULT_VARIABLE _module_path)
if (NOT _module_path)
  message(FATAL_ERROR
      "include could not find load file: AICxxProject\n"
      "1) Make sure that you have the git submodule https://github.com/CarloWood/cmake-aicxx added to the project.\n"
      "2) Use `include(cmake/aicxx/Project NO_POLICY_SCOPE)` right below the `project(...)` command in your top-level CMakeLists.txt."
  )
endif ()

if (NOT AICxxSubmodules)
  message(FATAL_ERROR
      "Please do not use `add_subdirectory(...)` for any of the AICxx modules.\n"
      "Instead, use `include(AICxxSubmodules)` in your top-level CMakeLists.txt."
  )
endif ()

# Is the utils submodule installed?
if (EXISTS "${CMAKE_SOURCE_DIR}/utils/has_print_on.h")
  # Check if the compiler supports the benchmark assembly code.
  try_compile(BENCHMARK_SUPPORTED
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake_benchmark_test.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cxx"
    CXX_STANDARD 20
    CMAKE_FLAGS "-DCMAKE_BUILD_TYPE=Release" "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR}"
    LOG_DESCRIPTION "Checking if benchmark asm is supported"
    OUTPUT_VARIABLE OUTPUT)

  if (BENCHMARK_SUPPORTED)
    message(STATUS "Benchmark code enabled")
  else ()
    message(STATUS "Benchmark code disabled")
    message(DEBUG "Compiling cmake_benchmark_test.cpp failed with ${OUTPUT}")
  endif ()
else ()
  message(STATUS "Benchmark code disabled: utils not present.")
endif ()

#==============================================================================
# POLICIES
#
if (POLICY CMP0167)
  # Use boost provided BoostConfig.cmake (requires boost version 1.70 or higher).
  # This policy was introduced in cmake 3.30.
  cmake_policy(SET CMP0167 NEW)
endif ()

#==============================================================================
# DEPENDENCIES
#

# We depend on these even when utils is not part of the project.

# Compile with -pthread.
find_package(Threads REQUIRED)

# debug_ostream_operators.h: #include <boost/shared_ptr.hpp>
#                            #include <boost/weak_ptr.hpp>
# tracked.h:                 #include <boost/range/adaptor/reversed.hpp>
find_package(Boost CONFIG REQUIRED)

# If utils is part of the project then we depend on it only if EnableDebugGlobal is ON.
# However, utils depends on cwds. To avoid a circular dependency we do never include
# the dependency on AICxx::utils. This is ok because utils is an OBJECT library and
# adding a dependency on it here would only cause its INTERFACE to be inherited,
# which happens to be Threads and Boost, which we depend on anyway, and the include
# directory of utils for sys.h and debug.h is the same, so we should get that too
# already (this means however that utils MUST be in project root, as does cwds).

#==============================================================================
# OPTIONS
#

# If the main project wants to use a different NAMESPACE_DEBUG, it must do:
#   # Tell cwds that we want to use another namespace.
#   set(NamespaceDebug other_namespace)
# in CMakeLists.txt in the root of the project.

message(STATUS "  in: NamespaceDebug = ${NamespaceDebug}")
set(OptionColorNamespaceDebug "${OptionColorYay}")
if (NamespaceDebug)
  set(OptionDefaultNamespaceDebug "")
else ()
  set(NamespaceDebug "debug")
  set(OptionDefaultNamespaceDebug " (default)")
endif ()
if (NOT "${NamespaceDebug}" STREQUAL "debug")
  set(NAMESPACE_DEBUG ${NamespaceDebug})
  set(OptionColorNamespaceDebug "${OptionColorAlert}")
endif ()

message(STATUS "${Option} ${OptionColor}NamespaceDebug${ColourReset} (value of NAMESPACE_DEBUG) =\n\t${OptionColorNamespaceDebug}${NamespaceDebug}${ColourReset}${OptionDefaultNamespaceDebug}")

# Specify cwds specific configure file.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h
               @ONLY)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(cwds_ObjLib OBJECT)

# The list of source files.
target_sources(cwds_ObjLib
  PRIVATE
    "debug.cxx"
    "sys.h"
    "debug.h"
)

if (BENCHMARK_SUPPORTED)

# Add the benchmark files to the project.
target_sources(cwds_ObjLib
  PRIVATE
    "benchmark.cxx"
    "benchmark.h"
)

# Always compile benchmark.cxx with -O3.
set_source_files_properties("benchmark.cxx" PROPERTIES COMPILE_OPTIONS "-O3")

else (BENCHMARK_SUPPORTED)

target_compile_definitions(cwds_ObjLib INTERFACE BENCHMARK_UNSUPPORTED)

endif (BENCHMARK_SUPPORTED)

# Required include search-paths.
target_include_directories(cwds_ObjLib
  PUBLIC
    "${top_objdir}"                             # Generated sys.h, debug.h and/or config.h, as well as for cwds/config.h.
    "${top_srcdir}"                             # For local sys.h and debug.h.
    "${CMAKE_CURRENT_SOURCE_DIR}"               # For fall-back sys.h and debug.h.
)

# Require support for at least C++17.
target_compile_features(cwds_ObjLib
  PUBLIC cxx_std_17
)

# Add dependencies.
target_link_libraries(cwds_ObjLib
  PUBLIC
    Boost::boost
    Threads::Threads
)

if (OptionEnableLibcwd)
  target_link_libraries(cwds_ObjLib
    PUBLIC
      ${libcwd_TARGET}
  )
endif ()

# This wasn't set as OptionEnableDebugGlobal yet because
# cwds is configured before utils, so cw_option for this
# option wasn't called yet.
if (EnableDebugGlobal)
  # Add a dependency on AICxx::utils because we need to include utils/config.h to get DEBUGGLOBAL.
  target_link_libraries(cwds_ObjLib
    PUBLIC
      AICxx::utils
  )
endif ()

# Create an ALIAS target.
add_library(AICxx::cwds ALIAS cwds_ObjLib)

# Prepend this object library to the list.
set(AICXX_OBJECTS_LIST AICxx::cwds ${AICXX_OBJECTS_LIST} CACHE INTERNAL "List of OBJECT libaries that this project uses.")
