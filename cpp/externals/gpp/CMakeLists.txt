cmake_minimum_required(VERSION 2.8.9)
project(gpp)

#### User configured external dependencies
# Sometimes cmake has difficulty finding Python (e.g., multiple installations, especially on OS X with
# system vs macport/homebrew versions); explicitly specify the location of Python when that happens.
# See documentation in FindPythonLibs.cmake for more information.

# User specifies what directory to look for Python headers in


# User specifies the location of the Python dynamic library


# User specifies priority Python versions to look for (in decreasing order of priority)

#### External dependencies

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

#### Sources
# Lists of source files that are dependencies. Currently we have one group for "core" functionality,
# one group for their test files, and one group for the C++<->Python interface.
# Do not include files with main() or BOOST_PYTHON_MODULE since no sources depend on those files.
# readonly
set(OPTIMAL_LEARNING_CORE_SOURCES
  gpp_covariance.cpp
  gpp_domain.cpp
  gpp_exception.cpp
  #gpp_heuristic_expected_improvement_optimization.cpp
  gpp_linear_algebra.cpp
  gpp_logging.cpp
  gpp_math.cpp
  gpp_expected_improvement_mcmc_optimization.cpp
  gpp_model_selection.cpp
  gpp_random.cpp
  #gpp_expected_improvement_gpu.cpp
  gpp_knowledge_gradient_optimization.cpp
  gpp_knowledge_gradient_mcmc_optimization.cpp
  )

# readonly
set(OPTIMAL_LEARNING_TEST_SOURCES
  gpp_covariance_test.cpp
  gpp_domain_test.cpp
  gpp_geometry_test.cpp
  #gpp_heuristic_expected_improvement_optimization_test.cpp
  gpp_linear_algebra_test.cpp
  gpp_math_test.cpp
  gpp_knowledge_gradient_optimization_test.cpp
  gpp_model_selection_test.cpp
  gpp_optimization_test.cpp
  gpp_random_test.cpp
  gpp_test_utils.cpp
  gpp_test_utils_test.cpp
  #gpp_expected_improvement_gpu_test.cpp
  )

# readonly
set(DEMO_NAMES
  demo_hyper
  demo_EI
  demo_full
  )

# readonly
#set(DEMO_SOURCES
#  gpp_hyperparameter_optimization_demo.cpp
#  gpp_expected_improvement_demo.cpp
#  gpp_hyper_and_EI_demo.cpp
#  )

#### Extra flags and definitions
# These are meant to be reasonable defaults. Users should feel free to alter the definitions (changing
# all targets) or append to individual targets as desired.

#### Extra link flags
# e.g., add -fprofile-arcs -ftest-coverage to use gcov
# This list need to be quoted.
# readonly
set(EXTRA_LINK_FLAGS "")

#### Extra compiler flags
# This list need to be quoted.
# readonly
if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # march=native is already set at this point and icc complains about overwriting it.
  set(EXTRA_COMPILE_FLAGS " -march=native")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Remove -march=native to prevent gcc from generating AVX (and newer) x86 extension instructions. OS X's GNU
  # assembler (as) is too old to parse these; they have been on Intel hardware since 2008 and AMD since 2011.
  # Power users: this can be overcome by replacing the program "as" with a script to invoke clang's assembler.
  # This can be valuable: performance gains from this one flag may be some 10-15%. Script example:
  # http://mac-os-forge.2317878.n4.nabble.com/gcc-as-AVX-binutils-and-MacOS-X-10-7-tp144472p144475.html
  string(REGEX REPLACE "-march=native" "" EXTRA_COMPILE_FLAGS ${EXTRA_COMPILE_FLAGS})
endif()

# Set some reasonable default extra flags for various compilers.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} -ftree-vectorize")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} -restrict -fstrict-aliasing -alias-const -ftz -no-prec-div -no-prec-sqrt -fast-transcendentals -qopt-subscript-in-range -fp-model fast=1")
endif()

#### Extra compiler definitions
# Do not quote these lists.
# readonly
set(EXTRA_COMPILE_DEFINITIONS_WARNING OL_WARNING_PRINT=1)
# readonly
set(EXTRA_COMPILE_DEFINITIONS_DEBUG OL_DEBUG_PRINT=1)

# readonly
set(EXTRA_COMPILE_DEFINITIONS "")
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS_DEBUG})
endif()

# Sets up an object library target. Object libraries essentially create a subdirectory with a
# "%.o" target in a Makefile. That is, it sets up targets for each specified source file but
# does not produce an archive. The resulting "library" is not linked, but it can be used as
# a dependency for other "real" cmake targets. This saves compile time.
# See: http://www.cmake.org/Wiki/CMake/Tutorials/Object_Library
#
# INPUTS
# name: the name of the object library; cmake will produce a subdirectory with this name
#       holding the object files and you refer to this name when specifying dependencies
# sources: list of source files to be compiled into the object library
# compile_flags: list of additional compilation flags to use
# compile_definitions: list of additional compilation definitions to use (DO NOT QUOTE THIS!)
function(configure_object_library name sources compile_flags compile_definitions)
  add_library(
    ${name} OBJECT
    ${sources}
    )
  set_target_properties(
    ${name} PROPERTIES
    COMPILE_FLAGS "${compile_flags}"
    COMPILE_DEFINITIONS "${compile_definitions}"
    )
endfunction(configure_object_library)

# Adds cmake executable targets for all specified name, source pairs.
# Each "source" file must have a main() function.
#
# Note: this creates object libraries for each executable source; for a source
#       named "foo.cpp", the object lib will be called "foo".
#
# INPUTS:
# exec_names: a list of names of executables
# exec_sources: a list of sources with main()
# compile_flags: list of additional compilation flags to use
# compile_definitions: list of additional compilation definitions to use (DO NOT QUOTE THIS!)
function(configure_exec_targets exec_names exec_sources dependencies compile_flags compile_definitions)
  list(LENGTH exec_names num_demos)
  math(EXPR num_demos "${num_demos} - 1")
  foreach(index RANGE ${num_demos})
    list(GET exec_names ${index} name)
    list(GET exec_sources ${index} source)

    # Drop the ".cpp" file ending to produce the library name.
    string(REGEX REPLACE "\\.cpp" "" object_name ${source})

    # C/C++'s ODR means we can only have one executable src per object lib.
    # This is a performance optimization: putting the excutable srcs in object
    # libs allows them to be compiled in parallel with all other source files.
    configure_object_library(
      ${object_name}
      ${source}
      "${compile_flags}"
      "${compile_definitions}"
      )

    add_executable(
      ${name}
      $<TARGET_OBJECTS:${object_name}>
      ${dependencies}
      )

    set_target_properties(
      ${name} PROPERTIES
      PREFIX ""
      COMPILE_FLAGS "${compile_flags}"
      COMPILE_DEFINITIONS "${compile_definitions}"
      LINK_FLAGS "${EXTRA_LINK_FLAGS}"
      )
  endforeach()
endfunction(configure_exec_targets)

#### Basic compiler flags
# Enable position independent code (gcc: -fPIC) by default.
# PIC is safer (code doesn't have to be located in the same place in memory every time) and all but
# required when generating 64-bit shared libraries.
# In x86-64, the well-known penalty from the x86 (32bit) days is all but gone; the addition of the new
# Relative Instruction Pointer addressing mode eliminates much of the previous overhead.
# http://eli.thegreenplace.net/2011/11/11/position-independent-code-pic-in-shared-libraries-on-x64/
# http://www.technovelty.org/c/position-independent-code-and-x86-64-libraries.html
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define some extra warnings (not enabled by -Wall -Wextra) for C/C++ that are nonetheless useful for guarding
# against potential errors, bad style, etc.
# Note: -Weffc++ is a useful one to check periodically, but its false positive rate is annoying.
# TODO(GH-151): With later versions of gcc, add: -Wuseless-cast -Wdate-time -Wfloat-conversion
set(EXTRA_C_WARNINGS " -Wundef -Wshadow -Wswitch-default -Wformat-nonliteral -Wpointer-arith -Wcast-align -Wcast-qual -Wwrite-strings -Wlogical-op")
set(EXTRA_C++_WARNINGS " -Wold-style-cast -Wnon-virtual-dtor -Wctor-dtor-privacy -Woverloaded-virtual -Wsign-promo -Wzero-as-null-pointer-constant -Wnoexcept -Wstrict-null-sentinel")

# Set baseline C++ flags; these should be good for everyone. The first two (C++14, OpenMP) are required.
# Set C++11
# Enable OpenMP
# Add extra warnings to guard against common error sources/bad style.
# Add (gcc) math flags; these strictly improve performance with no loss of "safety."

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # C4068: unknown pragma 'GCC'
  # C4464: relative include path contains '..'
  # C4514: unreferenced inline function has been removed
  # C4668: 'X' is not defined as a preprocessor macro 
  # C4820: 'x' bytes padding added after data member ...
  # C4996: 'fopen': This function or variable may be unsafe.
  # C5039: pointer or reference to potentially throwing function passed to 'extern "C"' function
  # C5045: Compiler will insert Spectre mitigation for memory load
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp /openmp:llvm /Wall /wd4068 /wd4464 /wd4514 /wd4668 /wd4820 /wd4996 /wd5039 /wd5045")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fopenmp -Wall -Wextra -fno-trapping-math -fno-math-errno")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-signaling-nans ${EXTRA_C_WARNINGS} ${EXTRA_C++_WARNINGS} -Wno-pedantic")
  endif()
endif()

# O3 takes longer to compile and the code produced is at best no faster than O2 (gcc, icc).
string(REGEX REPLACE "O3" "O2" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})


#### Object libraries
# See configure_object_library() function comments for more details.
# WARNING: You MUST have compatible flags set between OBJECT libraries and targets that depend on them!
# e.g., not setting -fPIC in OBJECT libraries makes then unusable in shared libs, setting -fpack-struct
# in one place but not another leads to binary incompatibility, etc.
configure_object_library(
  OPTIMAL_LEARNING_CORE_BUNDLE
  "${OPTIMAL_LEARNING_CORE_SOURCES}"
  "${EXTRA_COMPILE_FLAGS}"
  "${EXTRA_COMPILE_DEFINITIONS}"
  )
configure_object_library(
  OPTIMAL_LEARNING_TEST_BUNDLE
  "${OPTIMAL_LEARNING_TEST_SOURCES}"
  "${EXTRA_COMPILE_FLAGS}"
  "${EXTRA_COMPILE_DEFINITIONS}"
  )

# Rule for GPP library; includes dependency on having __init__.py in the same directory
add_library(
  gpp STATIC
  $<TARGET_OBJECTS:OPTIMAL_LEARNING_CORE_BUNDLE>
  $<TARGET_OBJECTS:OPTIMAL_LEARNING_TEST_BUNDLE>
  )

target_link_libraries(gpp ${Boost_LIBRARIES})
if (${MOE_USE_GPU} MATCHES "1")
    target_link_libraries(gpp ${CUDA_LIBRARIES} ${CMAKE_BINARY_DIR}/gpu/libOL_GPU.so)
endif()
        
# Make sure lib ends in ".so"
set_target_properties(
  gpp PROPERTIES
  PREFIX "lib"
  SUFFIX .a
  COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS}"
  COMPILE_DEFINITIONS "${EXTRA_COMPILE_DEFINITIONS}"
  LINK_FLAGS "${EXTRA_LINK_FLAGS}"
  )

set_target_properties(gpp OPTIMAL_LEARNING_CORE_BUNDLE OPTIMAL_LEARNING_TEST_BUNDLE PROPERTIES FOLDER "externals")

#### Demo executables
#set(dependencies $<TARGET_OBJECTS:OPTIMAL_LEARNING_CORE_BUNDLE> gpp_test_utils.cpp)
#configure_exec_targets(
#  "${DEMO_NAMES}"
#  "${DEMO_SOURCES}"
#  "${dependencies}"
#  "${EXTRA_COMPILE_FLAGS}"
#  "${EXTRA_COMPILE_DEFINITIONS}"
#  )

# Dummy target named "demos" that builds all demos
#add_custom_target(demos)
#add_dependencies(demos ${DEMO_NAMES})

