cmake_minimum_required(VERSION 3.16.3)

project(schedule)

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

file(GLOB schedule_files *.cc *.h)
add_library(schedule ${schedule_files})
target_link_libraries(schedule PUBLIC utils)

set_target_properties(schedule PROPERTIES FOLDER "schedule")