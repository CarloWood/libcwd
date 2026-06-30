# CTest driver that verifies <libexample/debug.h> triggers #error when it is
# included before the application's own "debug.h".
#
# The script receives the compiler and include paths as -D variables (because
# cmake -P mode has no access to the build-tree cache) and invokes the compiler
# directly.  It expects a non-zero exit code and checks that the compiler output
# contains the #error message from <libexample/debug.h>.
#
# Variables passed in with -D:
#   CXX            : the C++ compiler executable.
#   CWD_SRC_INC    : libcwd source include directory (for <libcwd/...>).
#   CWD_BLD_INC    : libcwd build include directory (for generated config.h).
#   EP_INC         : libexample include directory (for <libexample/...>).
#   SRC            : path to error_should_fail.cpp.

execute_process(
  COMMAND "${CXX}" -std=c++20 -DCWDEBUG
          "-I${CWD_SRC_INC}" "-I${CWD_BLD_INC}" "-I${EP_INC}"
          -c -o /dev/null "${SRC}"
  RESULT_VARIABLE result
  ERROR_VARIABLE stderr
  OUTPUT_VARIABLE stdout
)

if(result EQUAL 0)
  message(FATAL_ERROR
    "Expected error_should_fail.cpp to fail compilation, but it succeeded.\n"
    "This means the #error guard in <libexample/debug.h> did not fire when\n"
    "the file was included without first including \"debug.h\".")
endif()

if(NOT stderr MATCHES "must use.*#include.*debug\\.h")
  message(FATAL_ERROR
    "Compilation failed (as expected) but the #error message from\n"
    "<libexample/debug.h> was not found in the compiler output:\n"
    "${stderr}")
endif()

message(STATUS "library_pattern_error: #error correctly fired.")
