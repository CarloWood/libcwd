add_library(ctest_support STATIC support/test_support.cc)
target_include_directories(ctest_support
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(ctest_support PUBLIC cwd)
target_compile_options(ctest_support PRIVATE -g)

add_executable(basic_debug_output basic_debug_output.cc)
target_link_libraries(basic_debug_output PRIVATE ctest_support)
target_compile_options(basic_debug_output PRIVATE -g)

add_executable(colors_debug_output colors_debug_output.cc)
target_link_libraries(colors_debug_output PRIVATE ctest_support)
target_compile_options(colors_debug_output PRIVATE -g)

add_executable(dc dc.cc)
target_link_libraries(dc PRIVATE ctest_support)
target_compile_options(dc PRIVATE -g)

add_executable(hello_world_debug_channel hello_world_debug_channel.cc)
target_link_libraries(hello_world_debug_channel PRIVATE ctest_support)
target_compile_options(hello_world_debug_channel PRIVATE -g)

add_executable(mangling_symbols mangling_symbols.cc)
target_link_libraries(mangling_symbols PRIVATE ctest_support)
target_compile_options(mangling_symbols PRIVATE -g)

add_executable(location_lookup location_lookup.cc)
target_link_libraries(location_lookup PRIVATE ctest_support)
target_compile_options(location_lookup PRIVATE -g)

add_executable(location_cache_scenarios location_cache_scenarios.cc)
target_link_libraries(location_cache_scenarios PRIVATE ctest_support)
target_compile_options(location_cache_scenarios PRIVATE -g -O2)

add_executable(location_line_directives location_line_directives.cc)
target_link_libraries(location_line_directives PRIVATE ctest_support)
target_compile_options(location_line_directives PRIVATE -g)

add_executable(location_return_addresses location_return_addresses.cc)
target_link_libraries(location_return_addresses PRIVATE ctest_support)
target_compile_options(location_return_addresses PRIVATE -g)

add_executable(location_default_argument_functions location_default_argument_functions.cc support/location_default_argument_functions_cu.cc)
target_link_libraries(location_default_argument_functions PRIVATE ctest_support)
target_compile_options(location_default_argument_functions PRIVATE -g -finline)

add_executable(location_inlined_function_body location_inlined_function_body.cc)
target_link_libraries(location_inlined_function_body PRIVATE ctest_support)
target_compile_options(location_inlined_function_body PRIVATE -g -O2)

add_executable(location_from_global_object location_from_global_object.cc)
target_link_libraries(location_from_global_object PRIVATE ctest_support)
target_compile_options(location_from_global_object PRIVATE -g)

add_executable(symbol_range_overlaps symbol_range_overlaps.cc)
target_include_directories(symbol_range_overlaps
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(symbol_range_overlaps PRIVATE cwd)
target_compile_options(symbol_range_overlaps PRIVATE -g)

add_executable(nodebug_output nodebug_output.cc)
target_link_libraries(nodebug_output PRIVATE ctest_support)

add_executable(type_info type_info.cc)
target_link_libraries(type_info PRIVATE ctest_support)
target_compile_options(type_info PRIVATE -g)

add_executable(dlopen dlopen.cc)
target_link_libraries(dlopen PRIVATE ctest_support ${CMAKE_DL_LIBS})
target_compile_options(dlopen PRIVATE -g)
target_compile_definitions(dlopen PRIVATE "DLOPEN_MODULE_PATH=\"$<TARGET_FILE:dlopen_module>\"")

function(configure_location_loading_library target_name)
  target_include_directories(${target_name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/support
      ${CMAKE_BINARY_DIR}/include
      ${CMAKE_SOURCE_DIR}/include
  )
  target_compile_options(${target_name} PRIVATE -g)
  target_link_libraries(${target_name} PRIVATE cwd)
  set_target_properties(${target_name} PROPERTIES PREFIX "")
endfunction()

add_library(location_libtest2 SHARED support/location_libtest2.cc)
configure_location_loading_library(location_libtest2)

add_library(dlopen_module SHARED support/dlopen_module.cc)
target_include_directories(dlopen_module
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(dlopen_module PRIVATE cwd Threads::Threads)
target_compile_options(dlopen_module PRIVATE -g)
set_target_properties(dlopen_module PROPERTIES PREFIX "")
add_dependencies(dlopen dlopen_module)

add_library(location_plugin1 SHARED support/location_plugin1.cc)
configure_location_loading_library(location_plugin1)

add_library(location_plugin2 SHARED support/location_plugin2.cc)
configure_location_loading_library(location_plugin2)

add_library(location_libtest1 SHARED support/location_libtest1.cc)
configure_location_loading_library(location_libtest1)
target_link_libraries(location_libtest1 PRIVATE location_libtest2 ${CMAKE_DL_LIBS})
target_compile_definitions(location_libtest1 PRIVATE "LOCATION_PLUGIN1_PATH=\"$<TARGET_FILE:location_plugin1>\"")
add_dependencies(location_libtest1 location_plugin1)

add_executable(location_loading_fixture location_loading_fixture.cc)
target_link_libraries(location_loading_fixture PRIVATE ctest_support location_libtest1 ${CMAKE_DL_LIBS} AICxx::threadsafe)
target_compile_definitions(location_loading_fixture PRIVATE "LOCATION_PLUGIN2_PATH=\"$<TARGET_FILE:location_plugin2>\"")
add_dependencies(location_loading_fixture location_plugin2)

add_executable(lockable_auto_ptr lockable_auto_ptr.cc)
target_link_libraries(lockable_auto_ptr PRIVATE ctest_support)
target_compile_options(lockable_auto_ptr PRIVATE -g)

add_test(NAME basic_debug_output COMMAND basic_debug_output)
set_tests_properties(basic_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME colors_debug_output COMMAND colors_debug_output)
set_tests_properties(colors_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME dc COMMAND dc)
set_tests_properties(dc PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

set(hello_world_debug_channel_environment
  "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
  "LIBCWD_RCFILE_NAME=${CMAKE_SOURCE_DIR}/libcwdrc"
  "LIBCWD_RCFILE_OVERRIDE_NAME=${CMAKE_CURRENT_SOURCE_DIR}/support/libcwdrc_hello-override"
)
add_test(NAME hello_world_debug_channel COMMAND hello_world_debug_channel)
set_tests_properties(hello_world_debug_channel PROPERTIES
  ENVIRONMENT "${hello_world_debug_channel_environment}"
)

add_test(NAME mangling_symbols COMMAND mangling_symbols)
set_tests_properties(mangling_symbols PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_lookup COMMAND location_lookup)
set_tests_properties(location_lookup PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_cache_scenarios COMMAND location_cache_scenarios)
set_tests_properties(location_cache_scenarios PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_line_directives COMMAND location_line_directives)
set_tests_properties(location_line_directives PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_return_addresses COMMAND location_return_addresses)
set_tests_properties(location_return_addresses PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_default_argument_functions COMMAND location_default_argument_functions)
set_tests_properties(location_default_argument_functions PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_inlined_function_body COMMAND location_inlined_function_body)
set_tests_properties(location_inlined_function_body PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_from_global_object COMMAND location_from_global_object)
set_tests_properties(location_from_global_object PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME symbol_range_overlaps COMMAND symbol_range_overlaps)
set_tests_properties(symbol_range_overlaps PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME nodebug_output COMMAND nodebug_output)
set_tests_properties(nodebug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME type_info COMMAND type_info)
set_tests_properties(type_info PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME dlopen
  COMMAND ${CMAKE_COMMAND} -E env --unset=LIBCWD_PRINT_LOADING $<TARGET_FILE:dlopen>
)
set_tests_properties(dlopen PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$<TARGET_FILE_DIR:dlopen_module>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME location_loading_fixture
  COMMAND ${CMAKE_COMMAND} -E env --unset=LIBCWD_PRINT_LOADING $<TARGET_FILE:location_loading_fixture>
)
set_tests_properties(location_loading_fixture PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$<TARGET_FILE_DIR:location_libtest1>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME lockable_auto_ptr COMMAND lockable_auto_ptr)
set_tests_properties(lockable_auto_ptr PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)
