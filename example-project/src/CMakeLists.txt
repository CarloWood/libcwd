# The executable uses the project-local sys.h/debug.h wrappers from the parent directory,
# links against the cwds object-library target that is added by include(AICxxSubmodules),
# and inherits libcwd-related compile definitions and libraries through cwds when
# EnableLibcwd is active.

add_executable(helloworld
  helloworld.cpp
  debug.cpp
  ../debug.h
  ../sys.h
)

target_link_libraries(helloworld
  PRIVATE
    ${AICXX_OBJECTS_LIST}
)
