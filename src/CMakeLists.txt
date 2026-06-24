add_library(cwd SHARED)

math(EXPR INTERFACE_cwd_MAJOR_VERSION "${CW_INTERFACE_VERSION} - ${CW_INTERFACE_AGE}" OUTPUT_FORMAT DECIMAL)
set(INTERFACE_cwd_MINOR_VERSION ${CW_INTERFACE_AGE})
set(INTERFACE_cwd_PATCH_VERSION ${CW_INTERFACE_VERSION_REVISION})
set_target_properties(cwd PROPERTIES VERSION ${INTERFACE_cwd_MAJOR_VERSION}.${INTERFACE_cwd_MINOR_VERSION}.${INTERFACE_cwd_PATCH_VERSION})
set_target_properties(cwd PROPERTIES SOVERSION ${INTERFACE_cwd_MAJOR_VERSION})
# Note really sure what I'm doing here:
set_property(TARGET cwd PROPERTY INTERFACE_cwd_MAJOR_VERSION ${INTERFACE_cwd_MAJOR_VERSION})
set_property(TARGET cwd APPEND PROPERTY COMPATIBLE_INTERFACE_STRING cwd_MAJOR_VERSION)

target_sources(cwd
    PRIVATE
       "debug.cxx"
       "demangle.cxx"
       "dwarf.cxx"
       "environ.cxx"
       "location.cxx"
       "strerrno.cxx"
       "threading.cxx"
       "type_info.cxx"
)

target_compile_definitions(cwd
    PUBLIC
        "${cwd_compile_definitions}"
        VERSION="${PROJECT_VERSION}"
)

if (CW_BUILD_TYPE_IS_NOT_RELEASE)
  # Check if compiler is GCC, Clang, or AppleClang.
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(cwd PRIVATE -Wall -Wextra -Wpedantic -Wundef -Werror)
  endif ()
endif ()

target_include_directories(cwd
    PRIVATE
      "${CMAKE_BINARY_DIR}/include"
      "${CMAKE_SOURCE_DIR}/include"
      "${CMAKE_SOURCE_DIR}"
    INTERFACE
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(cwd
    PRIVATE
        Libcwd::utils
    PUBLIC
        Threads::Threads
    INTERFACE
        "${CMAKE_DL_LIBS}"
        "${LIBDW}"
        "${LIBELF}"
)
