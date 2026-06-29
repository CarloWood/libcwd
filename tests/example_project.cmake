# CTest driver that builds example-project as a standalone project and verifies
# the documented helloworld output verbatim.
#
# This script is run by ctest through `cmake -P`. It reproduces the exact recipe
# documented in example-project/README: configure the example project as an
# independent, out-of-source CMake project (i.e. NOT as a subdirectory of the
# libcwd build tree), build it against an *installed* libcwd, run the resulting
# helloworld executable with its rcfile override, and compare the combined
# stdout+stderr against the checked-in expected output file.
#
# The standalone configuration is the configuration that users actually use, and
# it is the only one that catches regressions where example-project no longer
# compiles on its own (for example, when a header starts requiring a newer C++
# standard than the example's own target_compile_features request).
#
# Variables passed in with -D:
#   CMAKE_COMMAND      : the cmake executable used for the nested configure/build.
#   EXAMPLE_SOURCE_DIR : path to the example-project source tree.
#   BUILD_DIR          : nested build directory (lives inside the outer build tree).
#   EXPECTED_FILE      : checked-in file with the exact expected combined output.
#   RCFILE_NAME        : value for the LIBCWD_RCFILE_OVERRIDE_NAME environment
#                        variable; a bare file name resolved relative to
#                        EXAMPLE_SOURCE_DIR (where example-project stores it).
#   PREFIX_PATH        : optional, forwarded to the nested build as CMAKE_PREFIX_PATH
#                        so find_package(libcwd) can locate a non-default install.

#------------------------------------------------------------------------------
# Build the optional -DCMAKE_PREFIX_PATH argument so the nested configure can
# reuse the same install prefix that made libcwd discoverable for this build.
set(prefix_arg "")
if(PREFIX_PATH)
  set(prefix_arg "-DCMAKE_PREFIX_PATH=${PREFIX_PATH}")
endif()

#------------------------------------------------------------------------------
# Step 1: configure the example project out-of-source, exactly as documented.
#
# Returns the nested cmake exit code in configure_result; cmake output is
# captured so it can be echoed only when configuration fails.
execute_process(
  COMMAND "${CMAKE_COMMAND}"
          -S "${EXAMPLE_SOURCE_DIR}" -B "${BUILD_DIR}"
          -DCMAKE_BUILD_TYPE=Debug
          -DExampleOption=ON
          ${prefix_arg}
  RESULT_VARIABLE configure_result
  OUTPUT_VARIABLE configure_stdout
  ERROR_VARIABLE configure_stderr
)

if(NOT configure_result EQUAL 0)
  message(STATUS "---- nested cmake configure stdout ----\n${configure_stdout}")
  message(STATUS "---- nested cmake configure stderr ----\n${configure_stderr}")
  message(FATAL_ERROR
    "Standalone configuration of example-project failed (exit code ${configure_result}). "
    "This usually means libcwd is not installed (see example-project/README) or that "
    "the installed libcwd cannot be found; pass its prefix via CMAKE_PREFIX_PATH.")
endif()

#------------------------------------------------------------------------------
# Determine how many build jobs to run in parallel.
#
# NUMBER_OF_LOGICAL_CORES counts hyperthreads too, which is the usual meaning
# of "all available CPUs"; the example project is tiny so there is no risk in
# requesting one job per logical core. Fall back to a single job if the host
# query fails to report a usable count.
cmake_host_system_information(RESULT _parallel_jobs QUERY NUMBER_OF_LOGICAL_CORES)
if(NOT _parallel_jobs OR _parallel_jobs LESS 1)
  set(_parallel_jobs 1)
endif()

#------------------------------------------------------------------------------
# Step 2: build the configured example project.
#
# Returns the build exit code in build_result. The build is incremental: a
# surviving BUILD_DIR is reconfigured and rebuilt, which keeps repeated test
# runs fast while still picking up source changes. --parallel spreads the
# handful of translation units across all logical cores instead of building
# them one at a time.
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config Debug
          --parallel "${_parallel_jobs}"
  RESULT_VARIABLE build_result
  OUTPUT_VARIABLE build_stdout
  ERROR_VARIABLE build_stderr
)

if(NOT build_result EQUAL 0)
  message(STATUS "---- nested cmake build stdout ----\n${build_stdout}")
  message(STATUS "---- nested cmake build stderr ----\n${build_stderr}")
  message(FATAL_ERROR
    "Standalone build of example-project failed (exit code ${build_result}).")
endif()

set(helloworld_exe "${BUILD_DIR}/src/helloworld")
if(NOT EXISTS "${helloworld_exe}")
  message(FATAL_ERROR
    "Build succeeded but '${helloworld_exe}' was not produced.")
endif()

#------------------------------------------------------------------------------
# Step 3: run helloworld with its rcfile override.
#
# The working directory must be EXAMPLE_SOURCE_DIR because LIBCWD_RCFILE_OVERRIDE_NAME
# is a bare file name that libcwd resolves relative to the current directory.
#
# stdout and stderr are captured separately. helloworld writes the two Dout lines
# to std::cerr (libcwd's debug stream, flushed as they happen) and the remaining
# three lines to std::cout (block-buffered because the output is a pipe, so it is
# flushed only at process exit). The combined, on-screen order is therefore
# deterministically all of stderr followed by all of stdout, which is why the
# expected file lists the CUSTOM/NOTICE lines first.
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
          "LIBCWD_RCFILE_OVERRIDE_NAME=${RCFILE_NAME}"
          "${helloworld_exe}"
  WORKING_DIRECTORY "${EXAMPLE_SOURCE_DIR}"
  RESULT_VARIABLE run_result
  OUTPUT_VARIABLE run_stdout
  ERROR_VARIABLE run_stderr
)

if(NOT run_result EQUAL 0)
  message(STATUS "---- helloworld stdout ----\n${run_stdout}")
  message(STATUS "---- helloworld stderr ----\n${run_stderr}")
  message(FATAL_ERROR
    "helloworld exited with code ${run_result}.")
endif()

# Reconstruct the true combined stderr+stdout order; see the comment above.
set(actual_output "${run_stderr}${run_stdout}")

#------------------------------------------------------------------------------
# Step 4: compare the combined output verbatim against the expected file.
file(READ "${EXPECTED_FILE}" expected_output)

if(NOT actual_output STREQUAL expected_output)
  # Dump both for easy diagnosis; also persist the actual output to a file in
  # the build tree so a byte-level diff can be run afterwards if needed.
  set(actual_dump "${BUILD_DIR}/helloworld_actual_output.txt")
  file(WRITE "${actual_dump}" "${actual_output}")
  message(STATUS "---- expected output ----\n${expected_output}")
  message(STATUS "---- actual output ----\n${actual_output}")
  message(STATUS "Actual output also written to: ${actual_dump}")
  message(FATAL_ERROR
    "helloworld output does not match the expected output verbatim.")
endif()

message(STATUS "example_project: output matches expected verbatim.")
