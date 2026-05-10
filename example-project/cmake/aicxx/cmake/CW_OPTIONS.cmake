# option cmake macro -- this file is part of cmake-aicxx.
#
# MIT License
# 
# Copyright (c) 2026 Carlo Wood <carlo@alinoe.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# This module should only be included from AICxxProject.
#
# It sets the following global variables:
#
# OptionEnableDebug                     - set when CMAKE_BUILD_TYPE is not Release and -DEnableDebug:BOOL=ON
#                                         (defaults to CW_BUILD_TYPE_IS_DEBUG || CW_BUILD_TYPE_IS_RELWITHDEBUG)
# OptionEnableLibcwd                    - set when CMAKE_BUILD_TYPE is not Release and OptionEnableDebug is true,
#                                         and -DEnableLibcwd:BOOL=ON (defaults to libcwd_r_FOUND)
#
# The following variables are intended to be used with the macro 'cw_option'
# defined below.
#
# CW_BUILD_TYPE_IS_RELEASE              - true iff CMAKE_BUILD_TYPE = Release
# CW_BUILD_TYPE_IS_NOT_RELEASE          - true iff CMAKE_BUILD_TYPE != Release
# CW_BUILD_TYPE_IS_RELWITHDEBINFO       - true iff CMAKE_BUILD_TYPE = RelWithDebInfo
# CW_BUILD_TYPE_IS_DEBUG                - true iff CMAKE_BUILD_TYPE = Debug
# CW_BUILD_TYPE_IS_BETATEST             - true iff CMAKE_BUILD_TYPE = BetaTest
# CW_BUILD_TYPE_IS_RELWITHDEBUG         - true iff CMAKE_BUILD_TYPE = RelWithDebug
# CW_BUILD_TYPE_IS_PERF                 - true iff CMAKE_BUILD_TYPE = Perf
# CW_BUILD_TYPE_IS_TRACY                - true iff CMAKE_BUILD_TYPE = Tracy
# CW_BUILD_TYPE_IS_NONE                 - true iff CMAKE_BUILD_TYPE = None
#
# Usage example,
#
#       # Option 'EnableDebug' compiles in debug mode.
#       cw_option(EnableDebug
#           "Build for debugging" ${CW_BUILD_TYPE_IS_DEBUG}
#           "CW_BUILD_TYPE_IS_NOT_RELEASE" OFF
#       )
#
# where argument four is a semi-colon separated list of variables
# that must be true for this option to be added (otherwise OFF,
# argument five).
include_guard(GLOBAL)
include(color_vars)
set(Option "${BoldCyan}Option${ColourReset}")
set(OptionColor "${Green}")
set(OptionColorYay "${Green}")
set(OptionColorAlert "${Red}")
set(OptionColorBuildType "${OptionColorAlert}")

# Clear INTERNAL cache values at start of project.
set(CW_BUILD_TYPE_IS_RELEASE ON CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_NOT_RELEASE OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_RELWITHDEBINFO OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_DEBUG OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_BETATEST OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_RELWITHDEBUG OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_PERF OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_TRACY OFF CACHE INTERNAL "")
set(CW_BUILD_TYPE_IS_NONE OFF CACHE INTERNAL "")
unset(OptionEnableDebug)
unset(OptionEnableLibcwd)

# Params: <option-name> <help-string> <default> <dependent-list> <default2>
#
# Sets 'Option<option-name>' to ON or OFF.
# If <option-name> is set to ON or OFF (from the commandline), then
# this is cached and reused the next time. Otherwise <default> is used.
#
# <dependent-list> is a list with zero or more variable names (semicolon
# separated) that must all be true however, or the value of
# 'Option<option-name>' is set to <default2> (defaults to OFF).
#
# option-name   : The name of the option. For example "EnableDebug".
# help-string   : The help string of the option with trailing period. For example "To enable debugging."
# default       : The uncached value to use when nothing was (previously) specified on the commandline.
# dependent-list: A semi-colon separated list of variables that all have to be true.
# default2      : The value to use when one or more of the variables in dependent-list is not true.
#
macro(cw_option option_name help_string default dependent_list default2)
  message(DEBUG "  in: ${option_name} = ${${option_name}}")
  set(extra_info "")
  set(forced_value FALSE)

  set(option_dependent_list_is_true true)
  if (NOT "${dependent_list}" STREQUAL "")
    foreach (OptionDependency ${dependent_list})
      if (NOT ${OptionDependency})
        set(option_dependent_list_is_true FALSE)
        string(SUBSTRING ${OptionDependency} 0 12 lead)
        if (lead STREQUAL "OptionEnable")
          set(value "OFF")
          string(SUBSTRING ${OptionDependency} 6 -1 name)
        else ()
          set(value "false")
          set(name ${OptionDependency})
        endif ()
        set(extra_info " (forced because ${name} is ${value})")
        set(forced_value true)
        message(DEBUG "  dependency: ${name} is ${value}; using argument 5 (${default2})")
        break()
      endif ()
    endforeach ()
  endif ()
  if (NOT option_dependent_list_is_true)
    # Use the <default2> parameter.
    if (${default2})
      set(Option${option_name} ON CACHE INTERNAL "")
    else ()
      set(Option${option_name} OFF CACHE INTERNAL "")
    endif ()
  elseif ("${${option_name}}" STREQUAL "ON" OR "${${option_name}}" STREQUAL "OFF")
    # Just (re-)set the help string.
    set(${option_name} "${${option_name}}" CACHE BOOL "${help_string}" FORCE)
    set(Option${option_name} ${${option_name}} CACHE INTERNAL "")
    set(extra_info " (cached)")
  else ()
    # Use the <default> that was passed.
    set(${option_name} "DEFAULT" CACHE STRING "${help_string}; can be ON or OFF")
    set(Option${option_name} ${default} CACHE INTERNAL "")
    set(extra_info " (default)")
  endif ()

  # Clobber the cached variable (as a normal variable) to make it harder to accidently use it.
  set(${option_name} "Use the variable Option${option_name} instead of ${option_name}.")
  if (forced_value)
    message(DEBUG "${Option} ${OptionColor}${option_name}${ColourReset} (${help_string}) =\n\t${Option${option_name}}${extra_info}")
    #message( DEBUG "${Option} ${option_name} (${help_string}) = ${Option${option_name}}${extra_info}" )
  elseif ((${Option${option_name}} AND ${default}) OR NOT (${Option${option_name}} OR ${default}))
    message(STATUS "${Option} ${OptionColor}${option_name}${ColourReset} (${help_string}) =\n\t${OptionColorYay}${Option${option_name}}${ColourReset}${extra_info}")
    #message( STATUS "${Option} ${option_name} (${help_string}) = ${OptionColorYay}${Option${option_name}}${ColourReset}${extra_info}" )
  else ()
    message(STATUS "${Option} ${OptionColor}${option_name}${ColourReset} (${help_string}) =\n\t${OptionColorAlert}${Option${option_name}}${ColourReset}${extra_info}")
    #message( STATUS "${Option} ${option_name} (${help_string}) = ${OptionColorAlert}${Option${option_name}}${ColourReset}${extra_info}" )
  endif ()
endmacro ()

# Normalize the build type capitalization and handle NONE case.
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
if ("${BUILD_TYPE_UPPER}" STREQUAL "RELEASE")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build Type" FORCE)
  set(OptionColorBuildType "${OptionColorYay}")
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "DEBUG")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build Type" FORCE)
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "RELWITHDEBINFO")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build Type" FORCE)
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "BETATEST")
  set(CMAKE_BUILD_TYPE "BetaTest" CACHE STRING "Build Type" FORCE)
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "RELWITHDEBUG")
  set(CMAKE_BUILD_TYPE "RelWithDebug" CACHE STRING "Build Type" FORCE)
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "PERF")
  set(CMAKE_BUILD_TYPE "Perf" CACHE STRING "Build Type" FORCE)
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "TRACY")
  set(CMAKE_BUILD_TYPE "Tracy" CACHE STRING "Build Type" FORCE)
elseif ("${BUILD_TYPE_UPPER}" STREQUAL "NONE")
  set(CMAKE_BUILD_TYPE "None" CACHE STRING "Build Type" FORCE)
else ()
  message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE \"${CMAKE_BUILD_TYPE}\".")
endif ()
set(default_enable_debug OFF)
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CW_BUILD_TYPE_IS_RELEASE OFF CACHE INTERNAL "")
  set(CW_BUILD_TYPE_IS_NOT_RELEASE ON CACHE INTERNAL "")
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(CW_BUILD_TYPE_IS_RELWITHDEBINFO ON CACHE INTERNAL "")
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CW_BUILD_TYPE_IS_DEBUG ON CACHE INTERNAL "")
  set(default_enable_debug ON)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "BetaTest")
  set(CW_BUILD_TYPE_IS_BETATEST ON CACHE INTERNAL "")
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebug")
  set(CW_BUILD_TYPE_IS_RELWITHDEBUG ON CACHE INTERNAL "")
  set(default_enable_debug ON)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Perf")
  set(CW_BUILD_TYPE_IS_PERF ON CACHE INTERNAL "")
  set(default_enable_debug ON)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Tracy")
  set(CW_BUILD_TYPE_IS_TRACY ON CACHE INTERNAL "")
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "None")
  set(CW_BUILD_TYPE_IS_NONE ON CACHE INTERNAL "")
endif ()
message(STATUS "${Option} ${OptionColor}CMAKE_BUILD_TYPE${ColourReset} =\n\t${OptionColorBuildType}${CMAKE_BUILD_TYPE}${ColourReset}")
#message(STATUS "${Option} ${OptionColor}CMAKE_BUILD_TYPE${ColourReset} = ${OptionColorBuildType}${CMAKE_BUILD_TYPE}${ColourReset}")

if (NOT "${PROJECT_NAME}" STREQUAL "libcwd")
  # Option 'EnableDebug' compiles in debug mode: we want debug code, debug output (if available),
  # asserts, debug info - but not necessarily no optimization.
  cw_option(EnableDebug
      "Build for debugging" ${default_enable_debug}
      "CW_BUILD_TYPE_IS_NOT_RELEASE" OFF)

  message(DEBUG "OptionEnableDebug is ${OptionEnableDebug}")

  if (CW_BUILD_TYPE_IS_NOT_RELEASE AND OptionEnableDebug)
    find_package(libcwd_r CONFIG)
    if (libcwd_r_FOUND)
      set(default_enable_libcwd ON)
      set(libcwd_r_TARGET "Libcwd::cwd_r")
    else ()
      set(default_enable_libcwd OFF)
    endif ()
  endif ()

  # Option 'EnableLibcwd' links with libcwd in debug mode.
  cw_option(EnableLibcwd
      "link with libcwd" "${default_enable_libcwd}"
      "CW_BUILD_TYPE_IS_NOT_RELEASE;OptionEnableDebug" OFF)

  if (OptionEnableLibcwd AND NOT libcwd_r_FOUND)
    message(FATAL_ERROR "EnableLibcwd specified but libcwd_r not found!")
  endif ()
endif ()
