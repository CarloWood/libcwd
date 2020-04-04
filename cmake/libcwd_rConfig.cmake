include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/libcwd_rTargets.cmake")

# Print 'Found' message exactly once.
get_property(_is_defined
  GLOBAL
  PROPERTY FIND_PACKAGE_CONFIG_MESSAGE_PRINTED_libcwd_r
  DEFINED
)
if(NOT _is_defined)
  set(FIND_PACKAGE_MESSAGE_DETAILS_libcwd_r CACHED INTERNAL "")
  set_property(
    GLOBAL
    PROPERTY FIND_PACKAGE_CONFIG_MESSAGE_PRINTED_libcwd_r 1
  )
endif()
find_package_message(
  libcwd_r
  "Found libcwd_r: ${libcwd_r_DIR} (found version \"${libcwd_r_VERSION}\")" "[${CMAKE_CURRENT_LIST_FILE}][${libcwd_r_DIR}][${libcwd_r_VERSION}]"
)
