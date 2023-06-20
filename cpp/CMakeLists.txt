cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
option(GET_CODE_COVERAGE "Get code coverage" OFF)
project(qiotoolkit)

include(build_types)
include(git_version)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # C4068: unknown pragma 'GCC'
  # C4464: relative include path contains '..'
  # C4514: unreferenced inline function has been removed
  # C4668: 'X' is not defined as a preprocessor macro 
  # C4820: 'x' bytes padding added after data member ...
  # C4996: 'fopen': This function or variable may be unsafe.
  # C5039: pointer or reference to potentially throwing function passed to 'extern "C"' function
  # C5045: Compiler will insert Spectre mitigation for memory load
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp /openmp:llvm /bigobj /Wall /wd4068 /wd4464 /wd4514 /wd4668 /wd4820 /wd4996 /wd5039 /wd5045")
  if(GET_CODE_COVERAGE STREQUAL "ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp /openmp:llvm /bigobj /Wall /wd4068 /wd4464 /wd4514 /wd4668 /wd4820 /wd4996 /wd5039 /wd5045 /analyze")
  endif()

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
  add_definitions(-Wall -Werror -Wextra)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -std=gnu++14 -fPIC -fopenmp -Wno-unknown-pragmas -pedantic")

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -ggdb3 -g3 -g")
  #Enabling this only in debug uilds
  if(GET_CODE_COVERAGE STREQUAL "ON")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer -march=native -mtune=native -O3 -Ofast -funroll-loops")

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # This matches both Clang and AppleClang
    # for apple-specific options, use STREQUAL "AppleClang" 
  else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
  endif()
endif()

enable_testing()

include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
add_subdirectory(utils)
add_subdirectory(app)
add_subdirectory(examples)
add_subdirectory(externals/gpp)
add_subdirectory(graph)
add_subdirectory(matcher)
add_subdirectory(markov)
add_subdirectory(model)
add_subdirectory(observe)
add_subdirectory(schedule)
add_subdirectory(solver)
add_subdirectory(strategy)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_custom_target(run_unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
    COMMAND bash ../run_tests.sh --configuration Debug --extension .exe)
  set_target_properties(run_unit_tests gmock gmock_main gtest gtest_main PROPERTIES FOLDER "test_utils")
else()
  add_custom_target(run_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
    COMMAND bash ../run_tests.sh)
    
    #sudo apt-get gcovr to use this function
  add_custom_target(run_coverage
    WORKING_DIRECTORY ${CMAKE_MODULE_PATH}
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug -DGET_CODE_COVERAGE=ON ${CMAKE_SOURCE_DIR}
    COMMAND bash ../run_tests.sh
    COMMAND gcovr -r ${CMAKE_SOURCE_DIR} . --html -o coverage.html --xml coverage.xml --exclude-directories ../externals --exclude-directories ../examples --gcov-exclude ./cmake/_deps --exclude-directories ../*/test )
endif()
