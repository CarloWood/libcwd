add_library(libcwd_utils_ObjLib OBJECT)

set_property(TARGET libcwd_utils_ObjLib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources(libcwd_utils_ObjLib
    PRIVATE
        "attach_gdb.cc"
        "buf2str.cc"
        "char2str.cc"
        "exec_prog.cc"
        "match.cc"
        "rcfile.cc"
)

target_include_directories(libcwd_utils_ObjLib
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_BINARY_DIR}/../include"
        # Include libcwd so that '#include "config.h"' works for the generated config.h.
        # Once installed that will work because it is in the same directory.
        "${CMAKE_CURRENT_BINARY_DIR}/../include/libcwd"
)

target_compile_definitions(libcwd_utils_ObjLib
    PRIVATE
        "${cwd_r_compile_definintions}"
)

target_link_libraries(libcwd_utils_ObjLib
    PRIVATE
        Threads::Threads
)

# Create an ALIAS target.
add_library(Libcwd::utils ALIAS libcwd_utils_ObjLib)
