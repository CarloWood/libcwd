add_library(libcwd_utils_ObjLib OBJECT)

set_property(TARGET libcwd_utils_ObjLib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources(libcwd_utils_ObjLib
    PRIVATE
        "attach_gdb.cxx"
        "buf2str.cxx"
        "char2str.cxx"
        "exec_prog.cxx"
        "match.cxx"
        "rcfile.cxx"
)

# Require support for C++20.
target_compile_features(libcwd_utils_ObjLib
  PUBLIC cxx_std_20
)

target_include_directories(libcwd_utils_ObjLib
    PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_BINARY_DIR}/include"
        # Include libcwd so that '#include "config.h"' works for the generated config.h.
        # Once installed that will work because it is in the same directory.
        "${CMAKE_BINARY_DIR}/include/libcwd"
)

target_compile_definitions(libcwd_utils_ObjLib
    PRIVATE
        "${cwd_compile_definitions}"
)

target_link_libraries(libcwd_utils_ObjLib
    PRIVATE
        Threads::Threads
)

# Create an ALIAS target.
add_library(Libcwd::utils ALIAS libcwd_utils_ObjLib)
