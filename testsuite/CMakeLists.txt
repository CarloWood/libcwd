set(CMAKE_BUILD_TYPE None)
set(CMAKE_CXX_FLAGS "")

if (BUILD_TESTING)
  add_subdirectory(ctest)
endif ()
