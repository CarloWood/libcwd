include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/libcwd_rTargets.cmake")

if(EXISTS "/home/carlo/projects/libcwd/libcwd/cwm4/cmake/dump_cmake_variables.cmake")
include("/home/carlo/projects/libcwd/libcwd/cwm4/cmake/dump_cmake_variables.cmake")
dump_cmake_variables(.)
endif()

find_package_message(libcwd_r "Found libcwd_r: ${CMAKE_CURRENT_LIST_FILE} (found version \"@PACKAGE_VERSION@\")" "[${CMAKE_CURRENT_LIST_FILE}][@PACKAGE_VERSION@]")
