find_program(DOXYGEN_EXECUTABLE doxygen)
# These two are needed for the tutorial.
find_program(M4_EXECUTABLE m4)
find_program(AWK_EXECUTABLE awk)

if (NOT DOXYGEN_EXECUTABLE OR NOT M4_EXECUTABLE OR NOT AWK_EXECUTABLE)
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND} -E echo "Documentation tools are missing: doxygen=${DOXYGEN_EXECUTABLE}, m4=${M4_EXECUTABLE}, awk=${AWK_EXECUTABLE}"
    COMMAND ${CMAKE_COMMAND} -E false
  )
  return()
endif ()

# Extract @LATEST_RELEASE_DATE@ from release-notes.md (which is in the first ## header).
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/release-notes.md")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/release-notes.md" release_note_headings REGEX "^## .+")
list(GET release_note_headings 0 latest_release_heading)
string(REGEX REPLACE "^##[ 	]+(.+)$" "\\1" LATEST_RELEASE_DATE "${latest_release_heading}")

# Replace @PACKAGE_NAME@ @PACKAGE_VERSION@ @srcdir@ @top_builddir@ and @top_srcdir@ in doxygen.config.in.
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(srcdir ${CMAKE_CURRENT_SOURCE_DIR})
set(top_builddir ${CMAKE_BINARY_DIR})
set(top_srcdir ${CMAKE_SOURCE_DIR})
configure_file(doxygen.config.in doxygen.config @ONLY)

# Replace @PACKAGE_VERSION@ and @LATEST_RELEASE_DATE@ in Introduction.md.in.
configure_file(Introduction.md.in Introduction.md @ONLY)

add_custom_target(doc-assets
  COMMAND ${CMAKE_COMMAND} -E rm -rf
          "${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome-css"
          "${CMAKE_CURRENT_BINARY_DIR}/doxygen-examples"
          "${CMAKE_CURRENT_BINARY_DIR}/images"
          "${CMAKE_CURRENT_BINARY_DIR}/scripts"
          "${CMAKE_CURRENT_BINARY_DIR}/www"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css" "${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome-css"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-examples" "${CMAKE_CURRENT_BINARY_DIR}/doxygen-examples"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/css" "${CMAKE_CURRENT_BINARY_DIR}/css"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/images" "${CMAKE_CURRENT_BINARY_DIR}/images"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/scripts" "${CMAKE_CURRENT_BINARY_DIR}/scripts"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/www" "${CMAKE_CURRENT_BINARY_DIR}/www"
)

set(doc_style_browsers netscape4 netscape6 mozilla konqueror ie)
set(doc_style_templates main.css doxygen.css tutorial.css tag-cw.css)
set(doc_style_outputs)
foreach(doc_style_browser ${doc_style_browsers})
  foreach(doc_style_template ${doc_style_templates})
    set(doc_style_output "${CMAKE_CURRENT_BINARY_DIR}/styles/${doc_style_browser}/${doc_style_template}")
    add_custom_command(
      OUTPUT "${doc_style_output}"
      COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_style.sh"
              "${CMAKE_C_COMPILER}"
              "${CMAKE_CURRENT_SOURCE_DIR}/styles"
              "${doc_style_browser}"
              "${doc_style_template}"
              "${doc_style_output}"
      DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/styles/${doc_style_template}"
        "${CMAKE_CURRENT_SOURCE_DIR}/styles/defines.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_style.sh"
      VERBATIM
    )
    list(APPEND doc_style_outputs "${doc_style_output}")
  endforeach()
endforeach()
add_custom_target(doc-styles DEPENDS ${doc_style_outputs})

set(doc_external_files
  "${CMAKE_CURRENT_BINARY_DIR}/external/INSTALL"
  "${CMAKE_CURRENT_BINARY_DIR}/external/sys.h"
  "${CMAKE_CURRENT_BINARY_DIR}/external/debug.h"
  "${CMAKE_CURRENT_BINARY_DIR}/external/libcwdrc"
)
add_custom_command(
  OUTPUT ${doc_external_files}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/external"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/INSTALL" "${CMAKE_CURRENT_BINARY_DIR}/external/INSTALL"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/example-project/sys.h" "${CMAKE_CURRENT_BINARY_DIR}/external/sys.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/example-project/debug.h" "${CMAKE_CURRENT_BINARY_DIR}/external/debug.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/libcwdrc" "${CMAKE_CURRENT_BINARY_DIR}/external/libcwdrc"
  DEPENDS
    "${CMAKE_SOURCE_DIR}/INSTALL"
    "${CMAKE_SOURCE_DIR}/example-project/sys.h"
    "${CMAKE_SOURCE_DIR}/example-project/debug.h"
    "${CMAKE_SOURCE_DIR}/libcwdrc"
  VERBATIM
)
add_custom_target(doc-external DEPENDS ${doc_external_files})

file(GLOB doc_reference_inputs CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_SOURCE_DIR}/include/libcwd/*.h"
  "${CMAKE_SOURCE_DIR}/include/libcwd/*.inl.h"
  "${CMAKE_SOURCE_DIR}/include/libcwd/private/*.h"
  "${CMAKE_SOURCE_DIR}/src/*.cxx"
)
add_custom_command(
  OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    "${CMAKE_CURRENT_BINARY_DIR}/topics.html"
    "${CMAKE_CURRENT_BINARY_DIR}/reference-manual.html"
    "${CMAKE_CURRENT_BINARY_DIR}/release-notes.html"
  COMMAND ${CMAKE_COMMAND} -E rm -rf reference-manual
  COMMAND /bin/sh -c "rm -f md_*release-notes*.html pages.html doxygen_crawl.html usergroup*.html group__book*.html group__downloading.html group__preparation.html group__page__why__macro.html group__chapter__custom__debug__h.html group__book__writing__intro.html group__chapter__custom__do.html group__chapter__nesting.html group__chapter__symbols__intro.html group__chapter__environment.html group__chapter__attach__gdb.html group__chapter__gdb.html chapter-getting-started.html chapter-getting-started.js chapter-writing-debug-output.html chapter-writing-debug-output.js chapter-symbols.html chapter-symbols.js chapter-runtime.html chapter-runtime.js"
  COMMAND ${DOXYGEN_EXECUTABLE} doxygen.config
  COMMAND /bin/sh -c "if test -f preparation.html; then mv preparation.html preparation.tmp && sed -e 's/href=\"debug_8h\.html\">debug\.h/href=\"#preparation_step2\">debug.h/' -e 's/\"sys\.h\"/\"<a class=\"code\" href=\"#preparation_step2\">sys.h<\\/a>\"/' preparation.tmp > preparation.html && rm preparation.tmp; fi"
  COMMAND /bin/sh -c "for i in `grep -l '<h2>Modules</h2>' *.html 2>/dev/null`; do mv \"$i\" \"$i.tmp\" && sed -e 's%<h2>Modules</h2>%<h2>Related</h2>%' \"$i.tmp\" > \"$i\" && rm \"$i.tmp\"; done"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/doxygen.config"
    "${CMAKE_CURRENT_BINARY_DIR}/Introduction.md"
    doc-assets
    doc-styles
    doc-external
    ${doc_reference_inputs}
  VERBATIM
)
add_custom_target(doc-site DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/index.html" "${CMAKE_CURRENT_BINARY_DIR}/topics.html" "${CMAKE_CURRENT_BINARY_DIR}/reference-manual.html" "${CMAKE_CURRENT_BINARY_DIR}/release-notes.html")

add_subdirectory(tutorial)

add_custom_target(doc
  DEPENDS
    doc-site
    doc-styles
    doc-tutorial
)
