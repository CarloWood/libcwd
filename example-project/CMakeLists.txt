cmake_minimum_required(VERSION 3.25)

project(example
  VERSION 1.0.0
  LANGUAGES CXX
  DESCRIPTION "Example project demonstrating a config.h file, cw_option and custom debug channel."
)

#==============================================================================
# This project uses aicxx modules.
include(cmake/aicxx/Project NO_POLICY_SCOPE)

#==============================================================================
# OPTIONS

# Option 'ExampleOption' is on be default in debug mode.
cw_option(ExampleOption
    "Demonstrate some CMake Option" ${CW_BUILD_TYPE_IS_DEBUG}
    "CW_BUILD_TYPE_IS_NOT_RELEASE" OFF
)

if (OptionExampleOption)
  message(STATUS "Yay! OptionExampleOption is ON!")
  set(USE_EXAMPLE42 42)
  set(USE_EXAMPLE 42)   # Normally you'd just use 1 here (this is a 0/1 define, see config.h.in).
endif ()

#==============================================================================
# CONFIG FILES

# Tell cwds/sys.h that we have a config.h.
add_definitions(-DHAVE_CONFIG_H)
include_directories(${top_objdir})
# End of CONFIG FILES section.

#==============================================================================

# Declare aicxx submodules.
include(AICxxSubmodules)

# Generate the config.h file with project information.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

add_subdirectory(src)
