cmake_minimum_required(VERSION 3.16.3)

project(graph)

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

file(GLOB graph_files *.cc *.h)
add_library(graph ${graph_files})
target_link_libraries(graph PUBLIC utils observe)

set_target_properties(graph PROPERTIES FOLDER "graph")

