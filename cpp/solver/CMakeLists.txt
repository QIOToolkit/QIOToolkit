cmake_minimum_required(VERSION 3.16.3)

project(solver)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################

enable_testing()
add_subdirectory(test)

################################################################################

file(GLOB solver_files *.cc *.h)
add_library(solver ${solver_files})
target_link_libraries(solver PUBLIC utils schedule strategy markov observe)

add_library(evaluation_counter evaluation_counter.cc)
target_link_libraries(evaluation_counter PUBLIC utils)

add_library(simulated_annealing simulated_annealing.h simulated_annealing.cc)
target_link_libraries(simulated_annealing PUBLIC utils schedule markov observe evaluation_counter)

set_target_properties(solver evaluation_counter simulated_annealing PROPERTIES FOLDER "solver")