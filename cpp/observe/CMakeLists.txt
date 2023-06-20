cmake_minimum_required(VERSION 3.16.3)

project(observe)

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

add_library(observer observer.h observer.cc)
target_link_libraries(observer PUBLIC component utils)

add_library(observable observable.h observable.cc)
target_link_libraries(observable PUBLIC component exception utils)

file(GLOB observe_files *.cc *.h)
add_library(observe ${observe_files})
target_link_libraries(observe PUBLIC utils)

set_target_properties(observer observable observe PROPERTIES FOLDER "observe")
