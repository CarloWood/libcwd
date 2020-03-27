include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/libcwd_rTargets.cmake")
message(STATUS "Yup, we get here")
