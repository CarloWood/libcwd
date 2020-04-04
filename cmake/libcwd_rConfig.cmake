include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/libcwd_rTargets.cmake")

get_property(_value GLOBAL PROPERTY FIND_PACKAGE_CONFIG_MESSAGE_PRINTED_libcwd_r)
if(NOT _value)
  set_property(GLOBAL PROPERTY FIND_PACKAGE_CONFIG_MESSAGE_PRINTED_libcwd_r 1)
  set(FIND_PACKAGE_MESSAGE_DETAILS_libcwd_r CACHED INTERNAL "")
endif()

find_package_message(
  libcwd_r
  "Found libcwd_r: ${libcwd_r_DIR} (found version \"${libcwd_r_VERSION}\")" "[${CMAKE_CURRENT_LIST_FILE}][${libcwd_r_DIR}][${libcwd_r_VERSION}]"
)
