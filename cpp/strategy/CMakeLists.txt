cmake_minimum_required(VERSION 3.16.3)

project(strategy)

#### External dependencies

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


remove_definitions(-Werror -pedantic -Wextra)

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

file(GLOB strategy_files *.cc *.h)
add_library(strategy ${strategy_files})
target_link_libraries(strategy PUBLIC utils gpp)

set_target_properties(strategy PROPERTIES FOLDER "strategy")
