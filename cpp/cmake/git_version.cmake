# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(DCMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/git_version.h.in
  ${CMAKE_BINARY_DIR}/generated/git_version.h
)

include_directories(${CMAKE_BINARY_DIR}/generated)
