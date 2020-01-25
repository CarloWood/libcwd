add_library(utils_ObjLib OBJECT)

set_property(TARGET utils_ObjLib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources(utils_ObjLib
    PRIVATE
        "attach_gdb.cc"
        "buf2str.cc"
        "char2str.cc"
        "exec_prog.cc"
        "match.cc"
        "rcfile.cc"
)

target_include_directories(utils_ObjLib
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

target_compile_definitions(utils_ObjLib
    PRIVATE
        "${cwd_r_compile_definintions}"
)

target_link_libraries(utils_ObjLib
    PRIVATE
        Threads::Threads
)

# Create an ALIAS target.
add_library(Libcwd::utils ALIAS utils_ObjLib)
