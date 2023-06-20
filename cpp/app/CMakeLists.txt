cmake_minimum_required(VERSION 3.16.3)

project(app)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################

include(rapidjson)
enable_testing()
add_subdirectory(test)
################################################################################

add_library(runner runner.h runner.cc)
target_link_libraries(runner PUBLIC
  utils 
  solver
  model
  rapidjson
  gpp
  strategy
  )

add_executable(qiotoolkit qiotoolkit.cc)
target_link_libraries(qiotoolkit PUBLIC utils runner gpp)

set_target_properties(runner qiotoolkit PROPERTIES FOLDER "app")
