find_program(DOXYGEN_EXECUTABLE doxygen)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# These two are needed for the tutorial.
find_program(M4_EXECUTABLE m4)
find_program(AWK_EXECUTABLE awk)

if (NOT DOXYGEN_EXECUTABLE OR NOT M4_EXECUTABLE OR NOT AWK_EXECUTABLE OR NOT Python3_EXECUTABLE)
  add_custom_target(doc
    COMMAND ${CMAKE_COMMAND} -E echo "Documentation tools are missing: doxygen=${DOXYGEN_EXECUTABLE}, m4=${M4_EXECUTABLE}, awk=${AWK_EXECUTABLE}, python3=${Python3_EXECUTABLE}"
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

# Files doxygen scans as documentation input, tracked so that ninja re-runs
# doxygen whenever any of them changes. Only patterns that actually match real
# files are listed; doxygen's FILE_PATTERNS (*.md, *.h, *.inl, *.cxx) are wider,
# but patterns that match nothing would only add noise. CONFIGURE_DEPENDS makes
# CMake re-glob at build time, so newly added (or removed) source files are
# picked up without a manual reconfigure. Any file doxygen reads that is missing
# from this list will silently produce stale documentation.
file(GLOB doc_reference_inputs CONFIGURE_DEPENDS
  # INPUT = @srcdir@ (doc/): the documentation prose lives in *.md.
  "${CMAKE_CURRENT_SOURCE_DIR}/*.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  # INPUT = @top_srcdir@/src: only *.cxx here (src/macros.h holds no doxygen
  # documentation, so src/*.h is intentionally not tracked).
  "${CMAKE_SOURCE_DIR}/src/*.cxx"
  # INPUT = @top_srcdir@/include/libcwd: *.h covers both the plain headers and
  # the *.inl.h inline headers (the latter end in .h).
  "${CMAKE_SOURCE_DIR}/include/libcwd/*.h"
  # INPUT = @top_srcdir@/include/libcwd/private.
  "${CMAKE_SOURCE_DIR}/include/libcwd/private/*.h"
  # INPUT = @top_builddir@/include/libcwd: the generated config.h and sys.h.
  # Doxygen must read the GENERATED files (not the *.in templates): the @VAR@
  # placeholders in config.h.in defeat doxygen's group/anchor resolution and
  # break every \ref to the enable_* configuration-option groups. Depending on
  # the generated files here is also what makes the dependency correct in both
  # directions -- an edit to config.h.in triggers a CMake re-run (configure_file
  # registers it) that regenerates config.h, and a change to a CMake option
  # regenerates config.h directly; either way this newer config.h retriggers
  # doxygen.
  "${CMAKE_BINARY_DIR}/include/libcwd/*.h"
)

# Files doxygen pulls in through EXAMPLE_PATH via \include/\snippet/\example.
# `nodebug.h` lives at the repo root and is \include'd by doc/custom-debug.h.md;
# the external/ files are staged into the build tree by the doc-external command
# above and are \include'd by doc/preparation.md. Listing the staged files here
# (rather than only the doc-external target) makes the dependency real instead of
# order-only, so edits to example-project/sys.h etc. retrigger doxygen.
set(doc_example_inputs
  "${CMAKE_SOURCE_DIR}/nodebug.h"
  ${doc_external_files}
)
file(GLOB doc_doxygen_examples CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-examples/*"
)

# Auxiliary files doxygen reads directly (not via INPUT): the HTML header/footer
# and the extra files/stylesheets baked into every generated page. These are
# referenced by name in doxygen.config.in (HTML_HEADER, HTML_FOOTER,
# HTML_EXTRA_FILES, HTML_EXTRA_STYLESHEET). Only the darkmode-toggle.js is listed
# by doxygen.config.in; the other doxygen-awesome-css/*.js files are loaded by
# the browser at view time (copied by the doc-assets target) and do not affect
# doxygen's HTML generation, so they are intentionally not tracked here.
set(doc_doxygen_aux_files
  "${CMAKE_CURRENT_SOURCE_DIR}/html.header"
  "${CMAKE_CURRENT_SOURCE_DIR}/html.footer"
  "${CMAKE_CURRENT_SOURCE_DIR}/images/doxygen.svg"
  "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js"
  "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/doxygen-awesome.css"
  "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/doxygen-awesome-sidebar-only.css"
  "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/doxygen-awesome-sidebar-only-darkmode-toggle.css"
)

set(doc_site_stamp "${CMAKE_CURRENT_BINARY_DIR}/doc-site.stamp")

# Doxygen's primary pages, declared as byproducts so that `make clean` removes
# them and so IDE generators know they belong to the doc-site stamp command.
# Only files that doxygen actually writes here may be listed: a byproduct that
# is never produced (or that the command deletes) makes ninja see a permanently
# missing output and needlessly re-run doxygen whenever the build graph is
# re-evaluated. Hence topics.html (deleted below by fix_navtree.py's cleanup)
# and reference-manual.html (doxygen emits the group page as
# group__reference-manual.html, not this name) are intentionally absent.
set(doc_site_byproducts
  "${CMAKE_CURRENT_BINARY_DIR}/index.html"
  "${CMAKE_CURRENT_BINARY_DIR}/release-notes.html"
)

add_custom_command(
  OUTPUT "${doc_site_stamp}"
  BYPRODUCTS ${doc_site_byproducts}
  COMMAND ${CMAKE_COMMAND} -E rm -rf reference-manual
  COMMAND ${DOXYGEN_EXECUTABLE} doxygen.config
  COMMAND /bin/sh -c "rm -f ${CMAKE_CURRENT_BINARY_DIR}/topics.html"
  COMMAND /bin/sh -c "for i in `grep -l '<h2>Modules</h2>' *.html 2>/dev/null`; do mv \"$i\" \"$i.tmp\" && sed -e 's%<h2>Modules</h2>%<h2>Related</h2>%' \"$i.tmp\" > \"$i\" && rm \"$i.tmp\"; done"
  COMMAND /bin/sh -c "echo 'NAVTREE[0][0] = \"libcwd 2.0\";' >> \"${CMAKE_CURRENT_BINARY_DIR}/navtreedata.js\""
  COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/scripts/fix_navtree.py
          ${CMAKE_CURRENT_BINARY_DIR}/navtreedata.js
  COMMAND ${CMAKE_COMMAND} -E touch "${doc_site_stamp}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/doxygen.config"
    "${CMAKE_CURRENT_BINARY_DIR}/Introduction.md"
    ${doc_reference_inputs}
    ${doc_example_inputs}
    ${doc_doxygen_examples}
    ${doc_doxygen_aux_files}
    doc-assets
    doc-styles
    doc-external
  VERBATIM
)

add_custom_target(doc-site
  DEPENDS "${doc_site_stamp}"
)

add_subdirectory(tutorial)

add_custom_target(doc
  DEPENDS
    doc-site
    doc-styles
    doc-external
    doc-tutorial
)
