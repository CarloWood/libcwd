# Manual regression/example programs. These targets are intentionally excluded
# from the default build and from CTest: they are small historical programs used
# for ad-hoc diagnostics, examples, and reproducing specific behavior. Build all
# of them with `cmake --build <build-dir> --target manual_tests`, or build an
# individual executable target by name.

set(MANUAL_TEST_TARGETS "")

function(add_manual_test name)
  add_executable(${name} EXCLUDE_FROM_ALL ${ARGN})
  target_include_directories(${name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}/include
      ${CMAKE_SOURCE_DIR}/include
  )
  target_compile_definitions(${name} PRIVATE CWDEBUG LIBCWD_THREAD_SAFE _REENTRANT)
  target_compile_options(${name} PRIVATE -g)
  target_link_libraries(${name} PRIVATE cwd_r Threads::Threads)
  set_property(GLOBAL APPEND PROPERTY LIBCWD_MANUAL_TEST_TARGETS ${name})
endfunction()

add_manual_test(location location.cc)
add_manual_test(lines lines.cc)
add_manual_test(inlined inlined.cc inlined_main.cc)
add_manual_test(threads threads.cc)
add_manual_test(threads2 threads2.cc)
add_manual_test(threads3 threads3.cc)
add_manual_test(threads4 threads4.cc)
add_manual_test(initbug initbug.cc initbug_GlobalObject.cc)
add_manual_test(syslog syslog.cc)

target_compile_options(inlined PRIVATE -finline)
target_compile_options(inlined PRIVATE -fpermissive)

get_property(MANUAL_TEST_TARGETS GLOBAL PROPERTY LIBCWD_MANUAL_TEST_TARGETS)
add_custom_target(manual_tests DEPENDS ${MANUAL_TEST_TARGETS})
