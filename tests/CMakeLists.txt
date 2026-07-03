add_library(ctest_support STATIC support/test_support.cpp)
target_include_directories(ctest_support
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(ctest_support PUBLIC cwd)
target_compile_options(ctest_support PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(basic_debug_output basic_debug_output.cpp)
target_link_libraries(basic_debug_output PRIVATE ctest_support)
target_compile_options(basic_debug_output PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(flush flush.cpp)
target_link_libraries(flush PRIVATE ctest_support)
target_compile_options(flush PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(continued continued.cpp)
target_link_libraries(continued PRIVATE ctest_support)
target_compile_options(continued PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(cf cf.cpp)
target_link_libraries(cf PRIVATE ctest_support)
target_compile_options(cf PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(demangler demangler.cpp)
target_link_libraries(demangler PRIVATE ctest_support)
target_compile_options(demangler PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(do do.cpp)
target_link_libraries(do PRIVATE ctest_support)
target_compile_options(do PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(colors_debug_output colors_debug_output.cpp)
target_link_libraries(colors_debug_output PRIVATE ctest_support)
target_compile_options(colors_debug_output PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(threaded_debug_output threaded_debug_output.cpp)
target_link_libraries(threaded_debug_output PRIVATE ctest_support)
target_compile_options(threaded_debug_output PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(concurrent_core_dump_threads concurrent_core_dump_threads.cpp)
target_link_libraries(concurrent_core_dump_threads PRIVATE ctest_support Threads::Threads)
target_compile_options(concurrent_core_dump_threads PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(dc dc.cpp)
target_link_libraries(dc PRIVATE ctest_support)
target_compile_options(dc PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(hello_world_debug_channel hello_world_debug_channel.cpp)
target_link_libraries(hello_world_debug_channel PRIVATE ctest_support)
target_compile_options(hello_world_debug_channel PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(mangling_symbols mangling_symbols.cpp)
target_link_libraries(mangling_symbols PRIVATE ctest_support)
target_compile_options(mangling_symbols PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(location_lookup location_lookup.cpp)
target_link_libraries(location_lookup PRIVATE ctest_support)
target_compile_options(location_lookup PRIVATE -g -Wall -Wextra -Wundef -Werror)

add_executable(location_cache_scenarios location_cache_scenarios.cpp)
target_link_libraries(location_cache_scenarios PRIVATE ctest_support)
target_compile_options(location_cache_scenarios PRIVATE -g -O2 -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(location_line_directives location_line_directives.cpp)
target_link_libraries(location_line_directives PRIVATE ctest_support)
target_compile_options(location_line_directives PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(location_return_addresses location_return_addresses.cpp)
target_link_libraries(location_return_addresses PRIVATE ctest_support)
target_compile_options(location_return_addresses PRIVATE -g -O0 -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(location_default_argument_functions location_default_argument_functions.cpp support/location_default_argument_functions_cu.cpp)
target_link_libraries(location_default_argument_functions PRIVATE ctest_support)
target_compile_options(location_default_argument_functions PRIVATE -g -O0 -Wall -Wextra -Wundef -Werror)

add_executable(location_inlined_function_body location_inlined_function_body.cpp)
target_link_libraries(location_inlined_function_body PRIVATE ctest_support)
target_compile_options(location_inlined_function_body PRIVATE -g -O2 -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(location_from_global_object location_from_global_object.cpp)
target_link_libraries(location_from_global_object PRIVATE ctest_support)
target_compile_options(location_from_global_object PRIVATE -g -Wall -Wextra -Wundef -Werror)

add_executable(symbol_range_overlaps symbol_range_overlaps.cpp)
target_include_directories(symbol_range_overlaps
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(symbol_range_overlaps PRIVATE cwd)
target_compile_options(symbol_range_overlaps PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(nodebug_output nodebug_output.cpp)
target_link_libraries(nodebug_output PRIVATE ctest_support)
target_compile_options(nodebug_output PRIVATE -g0 -Wall -Wextra -Wundef -Werror)

add_executable(type_info type_info.cpp)
target_link_libraries(type_info PRIVATE ctest_support)
target_compile_options(type_info PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

add_executable(dlopen dlopen.cpp)
target_link_libraries(dlopen PRIVATE ctest_support ${CMAKE_DL_LIBS})
target_compile_options(dlopen PRIVATE -g -O0 -Wall -Wextra -Wpedantic -Wundef -Werror)
target_compile_definitions(dlopen PRIVATE "DLOPEN_MODULE_PATH=\"$<TARGET_FILE:dlopen_module>\"")

function(configure_location_loading_library target_name)
  target_include_directories(${target_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/support
      ${CMAKE_BINARY_DIR}/include
      ${CMAKE_SOURCE_DIR}/include
  )
  target_compile_options(${target_name} PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)
  target_link_libraries(${target_name} PRIVATE cwd)
  set_target_properties(${target_name} PROPERTIES PREFIX "")
endfunction()

add_library(location_libtest2 SHARED support/location_libtest2.cpp)
configure_location_loading_library(location_libtest2)

add_library(dlopen_module SHARED support/dlopen_module.cpp)
target_include_directories(dlopen_module
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(dlopen_module PRIVATE cwd Threads::Threads)
target_compile_options(dlopen_module PRIVATE -g -O0 -Wall -Wextra -Wpedantic -Wundef -Werror)
set_target_properties(dlopen_module PROPERTIES PREFIX "")
add_dependencies(dlopen dlopen_module)

add_library(location_plugin1 SHARED support/location_plugin1.cpp)
configure_location_loading_library(location_plugin1)

add_library(location_plugin2 SHARED support/location_plugin2.cpp)
configure_location_loading_library(location_plugin2)

add_library(location_libtest1 SHARED support/location_libtest1.cpp)
configure_location_loading_library(location_libtest1)
target_link_libraries(location_libtest1 PRIVATE location_libtest2 ${CMAKE_DL_LIBS})
target_compile_definitions(location_libtest1 PRIVATE "LOCATION_PLUGIN1_PATH=\"$<TARGET_FILE:location_plugin1>\"")
add_dependencies(location_libtest1 location_plugin1)

add_executable(location_loading_fixture location_loading_fixture.cpp)
target_link_libraries(location_loading_fixture PRIVATE ctest_support location_libtest1 ${CMAKE_DL_LIBS} AICxx::threadsafe)
target_compile_definitions(location_loading_fixture PRIVATE "LOCATION_PLUGIN2_PATH=\"$<TARGET_FILE:location_plugin2>\"")
add_dependencies(location_loading_fixture location_plugin2)

add_executable(lockable_auto_ptr lockable_auto_ptr.cpp)
target_link_libraries(lockable_auto_ptr PRIVATE ctest_support)
target_compile_options(lockable_auto_ptr PRIVATE -g -Wall -Wextra -Wpedantic -Wundef -Werror)

# Specification test for the cwds API being absorbed into libcwd.
# See cwds_api_surface.cpp for the list of symbols it pins down.
add_executable(cwds_api_surface cwds_api_surface.cpp)
target_include_directories(cwds_api_surface PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(cwds_api_surface PRIVATE ctest_support)
target_compile_options(cwds_api_surface PRIVATE -g -Wall -Wextra -Wundef -Wpedantic -Werror)

add_test(NAME basic_debug_output COMMAND basic_debug_output)
set_tests_properties(basic_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME flush COMMAND flush)
set_tests_properties(flush PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME continued COMMAND continued)
set_tests_properties(continued PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME cf COMMAND cf)
set_tests_properties(cf PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME demangler COMMAND demangler)
set_tests_properties(demangler PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME do COMMAND do)
set_tests_properties(do PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME colors_debug_output COMMAND colors_debug_output)
set_tests_properties(colors_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME threaded_debug_output COMMAND threaded_debug_output)
set_tests_properties(threaded_debug_output PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

add_test(NAME concurrent_core_dump_threads COMMAND concurrent_core_dump_threads)
set_tests_properties(concurrent_core_dump_threads PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
  TIMEOUT 10
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

add_test(NAME cwds_api_surface COMMAND cwds_api_surface)
set_tests_properties(cwds_api_surface PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

# Build example-project as a standalone project (exactly as documented in
# example-project/README) and verify that its helloworld executable produces the
# expected output verbatim. Unlike every other test above, this one does NOT
# build inside the libcwd tree: it runs a nested `cmake` configure/build against
# an *installed* libcwd, which is the only configuration that catches the
# example-project failing to compile on its own. The driver script lives in
# example_project.cmake; the expected output is pinned by
# example_project_expected.txt.
#
# The nested build directory is kept under the outer build tree (never in the
# source tree) and persists across runs so that repeated invocations are fast.
set(_example_project_source_dir "${CMAKE_SOURCE_DIR}/example-project")
set(_example_project_build_dir "${CMAKE_BINARY_DIR}/example-project-build")
add_test(NAME example_project
  COMMAND ${CMAKE_COMMAND}
          -DCMAKE_COMMAND=${CMAKE_COMMAND}
          -DEXAMPLE_SOURCE_DIR=${_example_project_source_dir}
          -DBUILD_DIR=${_example_project_build_dir}
          -DEXPECTED_FILE=${CMAKE_CURRENT_SOURCE_DIR}/example_project_expected.txt
          -DRCFILE_NAME=libcwdrc_example-project
          -DPREFIX_PATH=${CMAKE_PREFIX_PATH}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/example_project.cmake
)
set_tests_properties(example_project PROPERTIES
  TIMEOUT 180
  RUN_SERIAL TRUE
)

#------------------------------------------------------------------------------
# Tests for the "Libraries" subsection of doc/reference-manual/custom-debug.h.md.
#
# These tests reproduce the three code snippets shown in that subsection (a
# library .cpp that defines a debug channel, an installed <libexample/debug.h>,
# and an internal non-installed debug.h) and verify that they compile and work
# correctly in both CWDEBUG and non-CWDEBUG builds.
#
# Three ctest entries are created:
#   library_pattern        — CWDEBUG build: verifies debug output, macros, and
#                            the LibexampleForAllDebugChannels iteration.
#   library_pattern_nodebug— non-CWDEBUG build: verifies that the no-op macros
#                            compile and produce zero output.
#   library_pattern_error  — verifies the #error guard in <libexample/debug.h>
#                            fires when the header is included before debug.h.
#------------------------------------------------------------------------------
set(_lp_dir "${CMAKE_CURRENT_SOURCE_DIR}/library_pattern")

# --- CWDEBUG library (links against cwd, gets -DCWDEBUG from it) -------------
add_library(libexample_cwdebug STATIC "${_lp_dir}/src/debug.cc")
target_include_directories(libexample_cwdebug
  PRIVATE
    "${_lp_dir}/internal"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/include"
  PUBLIC
    "${_lp_dir}/include"
)
target_link_libraries(libexample_cwdebug PUBLIC cwd)
target_compile_options(libexample_cwdebug PRIVATE -g -Wall -Wextra -Wpedantic -Werror)

# --- CWDEBUG application -----------------------------------------------------
add_executable(library_pattern "${_lp_dir}/app/main.cpp")
target_include_directories(library_pattern PRIVATE
  "${_lp_dir}/app"
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_BINARY_DIR}/include"
)
target_link_libraries(library_pattern PRIVATE libexample_cwdebug cwd)
target_compile_options(library_pattern PRIVATE -g -Wall -Wextra -Wpedantic -Werror)

add_test(NAME library_pattern COMMAND library_pattern)
set_tests_properties(library_pattern PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cwd>:$ENV{LD_LIBRARY_PATH}"
)

# --- Non-CWDEBUG library (no cwd link, no CWDEBUG define) -------------------
add_library(libexample_nodebug STATIC "${_lp_dir}/src/debug.cc")
target_include_directories(libexample_nodebug
  PRIVATE
    "${_lp_dir}/internal"
  PUBLIC
    "${_lp_dir}/include"
)
target_compile_options(libexample_nodebug PRIVATE -g -Wall -Wextra -Wpedantic -Werror)

# --- Non-CWDEBUG application -------------------------------------------------
add_executable(library_pattern_nodebug "${_lp_dir}/app/main_nodebug.cpp")
target_include_directories(library_pattern_nodebug PRIVATE "${_lp_dir}/app")
target_link_libraries(library_pattern_nodebug PRIVATE libexample_nodebug)
target_compile_options(library_pattern_nodebug PRIVATE -g -Wall -Wextra -Wpedantic -Werror)

add_test(NAME library_pattern_nodebug COMMAND library_pattern_nodebug)

# --- #error compile-failure test ---------------------------------------------
# Tries to compile error_should_fail.cpp (which includes <libexample/debug.h>
# without first including "debug.h") and verifies the #error fires.
add_test(NAME library_pattern_error
  COMMAND ${CMAKE_COMMAND}
          -DCXX=${CMAKE_CXX_COMPILER}
          -DCWD_SRC_INC=${CMAKE_SOURCE_DIR}/include
          -DCWD_BLD_INC=${CMAKE_BINARY_DIR}/include
          -DEP_INC=${_lp_dir}/include
          -DSRC=${_lp_dir}/app/error_should_fail.cpp
          -P ${CMAKE_CURRENT_SOURCE_DIR}/library_pattern_error.cmake
)
