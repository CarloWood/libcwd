include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/libcwdTargets.cmake")

# Print 'Found' message exactly once.
get_property(_is_defined
  GLOBAL
  PROPERTY FIND_PACKAGE_CONFIG_MESSAGE_PRINTED_libcwd
  DEFINED
)
if(NOT _is_defined)
  set(FIND_PACKAGE_MESSAGE_DETAILS_libcwd CACHED INTERNAL "")
  set_property(
    GLOBAL
    PROPERTY FIND_PACKAGE_CONFIG_MESSAGE_PRINTED_libcwd 1
  )
endif()
find_package_message(
  libcwd
  "Found libcwd: ${libcwd_DIR} (found version \"${libcwd_VERSION}\")" "[${CMAKE_CURRENT_LIST_FILE}][${libcwd_DIR}][${libcwd_VERSION}]"
)
