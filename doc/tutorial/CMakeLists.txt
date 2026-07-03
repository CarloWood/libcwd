# Tutorial documentation build.
#
# The tutorial is delivered as Doxygen markdown pages that share the unified
# Doxygen Awesome shell (see doc/CMakeLists.txt, which feeds these files to
# doxygen).  All pages except tut5 are static markdown authored in the source
# tree; doxygen reads them directly and nothing here generates them.
#
# tut5 is the one page that is *generated*, because it alone has to keep its
# shown program output in lock-step with the actual example programs.  The
# example programs are real source files in examples5/; this directory:
#
#   1. compiles each examples5/test*.cc into examples5/<name>-bin, and
#   2. runs moo.awk over tut5.in, which executes each binary at its
#      <!-- START OUTPUT <command> --> marker and splices the real output into
#      tut5.md (the markdown doxygen then renders).
#
# The displayed code in tut5 is pulled straight from those same source files by
# doxygen's \include, so the code the reader sees, the code that is compiled,
# and the output that is captured are all guaranteed to refer to the same program.

# The example programs.  test5.*.cc each become an executable; debug.cc holds
# the four channel definitions shared by all of them.
set(tutorial_debug_source "${CMAKE_CURRENT_SOURCE_DIR}/examples5/debug.cc")
file(GLOB tutorial_test_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/examples5/test*.cc"
)

set(tutorial_include_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/examples5"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples5/sysinclude"
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_BINARY_DIR}/include"
)

# debug.cc is compiled once into an object library and linked into every test
# program; it holds the definitions of the four example debug channels.
add_library(doc-tutorial-examples5-debug OBJECT EXCLUDE_FROM_ALL "${tutorial_debug_source}")
target_compile_definitions(doc-tutorial-examples5-debug PRIVATE CWDEBUG)
target_include_directories(doc-tutorial-examples5-debug PRIVATE ${tutorial_include_directories})

# Build one executable per test5.*.cc example.  These are the programs whose
# output moo.awk runs and captures below, so tut5.md can only be generated once
# they all exist.
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
  add_dependencies(${tutorial_target} doc-tutorial-examples5-debug)
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

# Generate tut5.md from tut5.in by running each compiled example at the
# <!-- START OUTPUT <command> --> markers and splicing its real output in place.
# Everything else in tut5.in (the markdown prose and the \include directives) is
# passed through verbatim.  WORKING_DIRECTORY is the build tutorial dir so the
# `examples5/<binary>` references inside the markers resolve.
set(tutorial_tut5_md "${CMAKE_CURRENT_BINARY_DIR}/tut5.md")
add_custom_command(
  OUTPUT "${tutorial_tut5_md}"
  COMMAND /bin/sh -c "\"${AWK_EXECUTABLE}\" -f \"${CMAKE_CURRENT_SOURCE_DIR}/moo.awk\" \"${CMAKE_CURRENT_SOURCE_DIR}/tut5.in\" > \"${tutorial_tut5_md}\""
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tut5.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/moo.awk"
    doc-tutorial-examples
  VERBATIM
)

# doc-tutorial guarantees the generated tut5.md exists; doxygen (run by the
# parent doc-site target, which depends on this target) then reads it.
add_custom_target(doc-tutorial DEPENDS "${tutorial_tut5_md}")
