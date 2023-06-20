cmake_minimum_required(VERSION 3.16.3)

project(utils)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################

enable_testing()
add_subdirectory(test)

################################################################################

include(rapidjson)
include(pcg)
include(proto)

################################################################################
add_proto(problem_proto problem.proto)

file(GLOB utils_files *.cc *.h)
add_library(utils ${utils_files})

target_link_libraries(utils PUBLIC pcg rapidjson problem_proto)

set_target_properties(utils problem_proto PROPERTIES FOLDER "utils")