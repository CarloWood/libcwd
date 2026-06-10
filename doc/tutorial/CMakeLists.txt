set(tutorial_definition_file "${CMAKE_CURRENT_SOURCE_DIR}/../definitions.m4")

set(tutorial_static_pages
  index
  faq
  intro
  tut1
  tut2
  tut3
  tut4
  tut6
)

set(tutorial_html_outputs)
foreach(tutorial_page ${tutorial_static_pages})
  set(tutorial_input "${CMAKE_CURRENT_SOURCE_DIR}/${tutorial_page}.m4")
  set(tutorial_output "${CMAKE_CURRENT_BINARY_DIR}/${tutorial_page}.html")
  add_custom_command(
    OUTPUT "${tutorial_output}"
    COMMAND /bin/sh -c "\"${M4_EXECUTABLE}\" -D__TUTORIAL=1 -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\" \"${tutorial_input}\" > \"${tutorial_output}\""
    DEPENDS
      "${tutorial_input}"
      "${tutorial_definition_file}"
    VERBATIM
  )
  list(APPEND tutorial_html_outputs "${tutorial_output}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/tut5.in" tutorial_code_markers REGEX "<!-- START CODE .+ -->")
set(tutorial_generated_sources)
set(tutorial_test_sources)
foreach(tutorial_marker ${tutorial_code_markers})
  string(REGEX REPLACE "^.*<!-- START CODE ([^ ]+) -->.*$" "\\1" tutorial_block "${tutorial_marker}")
  set(tutorial_output "${CMAKE_CURRENT_BINARY_DIR}/${tutorial_block}")
  add_custom_command(
    OUTPUT "${tutorial_output}"
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/extract_tutorial_block.sh" "${CMAKE_CURRENT_SOURCE_DIR}/tut5.in" "${tutorial_block}" "${tutorial_output}"
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/tut5.in"
      "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/extract_tutorial_block.sh"
    VERBATIM
  )
  set_source_files_properties("${tutorial_output}" PROPERTIES GENERATED TRUE)
  list(APPEND tutorial_generated_sources "${tutorial_output}")
  if (tutorial_block MATCHES "^examples5/test.*\\.cc$")
    list(APPEND tutorial_test_sources "${tutorial_output}")
  endif ()
endforeach()

add_custom_target(doc-tutorial-sources DEPENDS ${tutorial_generated_sources})

set(tutorial_include_directories
  "${CMAKE_CURRENT_BINARY_DIR}/examples5"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples5"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples5/sysinclude"
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_BINARY_DIR}/include"
)

add_library(doc-tutorial-examples5-debug OBJECT EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/examples5/debug.cc")
add_dependencies(doc-tutorial-examples5-debug doc-tutorial-sources)
target_compile_definitions(doc-tutorial-examples5-debug PRIVATE CWDEBUG)
target_include_directories(doc-tutorial-examples5-debug PRIVATE ${tutorial_include_directories})

set(tutorial_example_targets)
foreach(tutorial_test_source ${tutorial_test_sources})
  get_filename_component(tutorial_test_name "${tutorial_test_source}" NAME)
  string(REGEX REPLACE "\\.cc$" "" tutorial_test_name "${tutorial_test_name}")
  string(MAKE_C_IDENTIFIER "${tutorial_test_name}" tutorial_target_suffix)
  set(tutorial_target "doc-tutorial-${tutorial_target_suffix}")
  add_executable(${tutorial_target} EXCLUDE_FROM_ALL
    "${tutorial_test_source}"
    $<TARGET_OBJECTS:doc-tutorial-examples5-debug>
  )
  add_dependencies(${tutorial_target} doc-tutorial-sources doc-tutorial-examples5-debug)
  target_compile_definitions(${tutorial_target} PRIVATE CWDEBUG)
  target_include_directories(${tutorial_target} PRIVATE ${tutorial_include_directories})
  target_link_libraries(${tutorial_target} PRIVATE cwd)
  set_target_properties(${tutorial_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/examples5"
    OUTPUT_NAME "${tutorial_test_name}-bin"
  )
  list(APPEND tutorial_example_targets ${tutorial_target})
endforeach()

add_custom_target(doc-tutorial-examples DEPENDS ${tutorial_example_targets})

set(tutorial_tut5_m4 "${CMAKE_CURRENT_BINARY_DIR}/tut5.m4")
add_custom_command(
  OUTPUT "${tutorial_tut5_m4}"
  COMMAND /bin/sh -c "\"${AWK_EXECUTABLE}\" -f \"${CMAKE_CURRENT_SOURCE_DIR}/moo.awk\" \"${CMAKE_CURRENT_SOURCE_DIR}/tut5.in\" > \"${tutorial_tut5_m4}\""
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tut5.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/moo.awk"
    doc-tutorial-examples
  VERBATIM
)

set(tutorial_tut5_html "${CMAKE_CURRENT_BINARY_DIR}/tut5.html")
add_custom_command(
  OUTPUT "${tutorial_tut5_html}"
  COMMAND /bin/sh -c "\"${M4_EXECUTABLE}\" -D__TUTORIAL=1 -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\" \"${tutorial_tut5_m4}\" > \"${tutorial_tut5_html}\""
  DEPENDS
    "${tutorial_tut5_m4}"
    "${tutorial_definition_file}"
  VERBATIM
)
list(APPEND tutorial_html_outputs "${tutorial_tut5_html}")

add_custom_target(doc-tutorial DEPENDS ${tutorial_html_outputs})
