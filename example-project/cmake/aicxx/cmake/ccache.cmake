cmake_minimum_required(VERSION 3.4...3.27)

# Cause cmake to use ccache for CXX.
if (DEFINED ENV{CCACHE_DIR})
  find_program(CCACHE_PROGRAM ccache)
  if (CCACHE_PROGRAM)
    # Support for Unix Makefiles and Ninja.
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ${CCACHE_PROGRAM} with CCACHE_DIR = \"$ENV{CCACHE_DIR}\".")
  else ()
    message(FATAL_ERROR "Environment variable CCACHE_DIR is set but cannot find program ccache!")
  endif ()
endif ()
