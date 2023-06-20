cmake_minimum_required(VERSION 3.16.3)

project(model)

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

file(GLOB model_files *.cc *.h)
list(REMOVE_ITEM model_files base_model.h base_model.cc model_registry.h)
add_library(model ${model_files})
target_link_libraries(model PUBLIC utils graph markov base_model)

add_library(base_model base_model.h base_model.cc model_registry.h)
target_link_libraries(base_model PUBLIC utils)

set_target_properties(model base_model PROPERTIES FOLDER "model")
