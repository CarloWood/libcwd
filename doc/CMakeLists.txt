find_program(DOXYGEN_EXECUTABLE doxygen)
find_program(M4_EXECUTABLE m4)
find_program(AWK_EXECUTABLE awk)

if (NOT DOXYGEN_EXECUTABLE OR NOT M4_EXECUTABLE OR NOT AWK_EXECUTABLE)
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND} -E echo "Documentation tools are missing: doxygen=${DOXYGEN_EXECUTABLE}, m4=${M4_EXECUTABLE}, awk=${AWK_EXECUTABLE}"
    COMMAND ${CMAKE_COMMAND} -E false
  )
  return()
endif ()

set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(top_srcdir ${CMAKE_SOURCE_DIR})
set(srcdir ${CMAKE_CURRENT_SOURCE_DIR})
set(top_builddir ${CMAKE_BINARY_DIR})

configure_file(doxygen.config.in doxygen.config @ONLY)
configure_file(html.header.in html.header @ONLY)
configure_file(html.footer.in html.footer @ONLY)

add_custom_target(doc-assets
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

file(GLOB doc_reference_inputs
  "${CMAKE_CURRENT_SOURCE_DIR}/*.dox"
  "${CMAKE_SOURCE_DIR}/include/libcwd/*.h"
  "${CMAKE_SOURCE_DIR}/include/libcwd/*.inl.h"
  "${CMAKE_SOURCE_DIR}/include/libcwd/private/*.h"
  "${CMAKE_SOURCE_DIR}/src/*.cxx"
)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/reference-manual/index.html"
  COMMAND ${CMAKE_COMMAND} -E rm -rf reference-manual
  COMMAND ${DOXYGEN_EXECUTABLE} doxygen.config
  COMMAND /bin/sh -c "if test -f reference-manual/preparation.html; then mv reference-manual/preparation.html reference-manual/preparation.tmp && sed -e 's/href=\"debug_8h\.html\">debug\.h/href=\"#preparation_step2\">debug.h/' -e 's/\"sys\.h\"/\"<a class=\"code\" href=\"#preparation_step2\">sys.h<\\/a>\"/' reference-manual/preparation.tmp > reference-manual/preparation.html && rm reference-manual/preparation.tmp; fi"
  COMMAND /bin/sh -c "for i in `grep -l '<h2>Modules</h2>' reference-manual/*.html 2>/dev/null`; do mv \"$i\" \"$i.tmp\" && sed -e 's%<h2>Modules</h2>%<h2>Related</h2>%' \"$i.tmp\" > \"$i\" && rm \"$i.tmp\"; done"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/doxygen.config"
    "${CMAKE_CURRENT_BINARY_DIR}/html.header"
    "${CMAKE_CURRENT_BINARY_DIR}/html.footer"
    doc-assets
    doc-styles
    doc-external
    ${doc_reference_inputs}
  VERBATIM
)
add_custom_target(doc-reference-manual DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/reference-manual/index.html")

add_subdirectory(tutorial)

add_custom_target(doc
  DEPENDS
    doc-reference-manual
    doc-styles
    doc-tutorial
)
