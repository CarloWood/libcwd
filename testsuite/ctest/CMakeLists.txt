add_library(ctest_support STATIC support/test_support.cc)
target_include_directories(ctest_support
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(ctest_support PUBLIC cwd_r)
target_compile_options(ctest_support PUBLIC -g)

add_executable(basic_debug_output basic_debug_output.cc)
target_link_libraries(basic_debug_output PRIVATE ctest_support)

add_executable(colors_debug_output colors_debug_output.cc)
target_link_libraries(colors_debug_output PRIVATE ctest_support)

add_executable(hello_world_debug_channel hello_world_debug_channel.cc)
target_link_libraries(hello_world_debug_channel PRIVATE ctest_support)

add_executable(mangling_symbols mangling_symbols.cc)
target_link_libraries(mangling_symbols PRIVATE ctest_support)

add_test(NAME basic_debug_output COMMAND basic_debug_output)
set_tests_properties(basic_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd_r>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME colors_debug_output COMMAND colors_debug_output)
set_tests_properties(colors_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd_r>:$ENV{LD_LIBRARY_PATH}"
)

set(hello_world_debug_channel_environment
  "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd_r>:$ENV{LD_LIBRARY_PATH}"
  "LIBCWD_RCFILE_NAME=${CMAKE_SOURCE_DIR}/libcwdrc"
  "LIBCWD_RCFILE_OVERRIDE_NAME=${CMAKE_CURRENT_SOURCE_DIR}/support/libcwdrc_hello-override"
)
add_test(NAME hello_world_debug_channel COMMAND hello_world_debug_channel)
set_tests_properties(hello_world_debug_channel PROPERTIES
  ENVIRONMENT "${hello_world_debug_channel_environment}"
)

add_test(NAME mangling_symbols COMMAND mangling_symbols)
set_tests_properties(mangling_symbols PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd_r>:$ENV{LD_LIBRARY_PATH}"
)
