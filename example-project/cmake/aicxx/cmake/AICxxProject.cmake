# Check if AICxxProject was included correctly.
if (NOT top_srcdir)
  message(FATAL_ERROR
    "Add the following to the top of the CMakeLists.txt file, right after the `project(...)` command, in the root of the project:\n"
    "  include(cmake/aicxx/Project NO_POLICY_SCOPE)    # <=== ADD THIS\n" )
endif ()

# Print the current subdirectory, relative to the project root.
file(RELATIVE_PATH current_subdirectory "${top_srcdir}" "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "----------------------------------------------------\n** Configuring subdirectory ${current_subdirectory}:")

# Add support for CMAKE_BUILD_TYPE, EnableDebug, EnableGlobalDebug, EnableLibcwd
include(CW_OPTIONS)
