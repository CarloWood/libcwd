include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/libcwd_rTargets.cmake")
find_package_message(libcwd_r "Found libcwd_r: ${CMAKE_CURRENT_LIST_FILE} (found version \"@PACKAGE_VERSION@\")" "[${CMAKE_CURRENT_LIST_FILE}][@PACKAGE_VERSION@]")
