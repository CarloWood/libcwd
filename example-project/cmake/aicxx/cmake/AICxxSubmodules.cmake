# This should be included (only) from the top level CMakeLists.txt file.
include_guard(GLOBAL)

# This is a list of all aicxx submodules that exist.
# If a submodule X depends on Y it must be on the right, i.e.: Y X.
set(AICxxSubmodules cwds utils memory xml events threadsafe threadpool evio statefultask fastprimes math cairowindow)

foreach (subdir ${AICxxSubmodules})
  get_filename_component(_fullpath "${subdir}" REALPATH)
  if (EXISTS "${_fullpath}" AND EXISTS "${_fullpath}/CMakeLists.txt")
#[[
    if ( EXISTS "${_fullpath}/CMpackages.cmake" )
      include( "${_fullpath}/CMpackages.cmake" )
    endif ()
#]]
    add_subdirectory( ${subdir} )
  endif ()
endforeach ()
