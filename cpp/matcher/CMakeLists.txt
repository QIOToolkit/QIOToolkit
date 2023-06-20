cmake_minimum_required(VERSION 3.16.3)

project(matcher)

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

add_library(matchers INTERFACE)
target_link_libraries(matchers INTERFACE utils)