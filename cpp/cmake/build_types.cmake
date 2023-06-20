# Build Types
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
    CACHE STRING "Choose the type of build, options are: None Debug Release SelfConsistency RelWithDebInfo MinSizeRel tsan asan lsan msan ubsan gprof"
    FORCE)

# SelfConsistency Test
#
# This build mode runs each cost_difference against a full evaluation of the whole
# cost function before and after an accepted change. This is extermely slow and
# intended only to validate that the two methods are consistent with each other.
#
set (CMAKE_CXX_FLAGS_SELFCONSISTENCY
    "-g -O0 -DDEBUG_SELFCONSISTENCY"
    CACHE STRING "Flags used by the C compiler to build a binary with self-consistency checks during cost updates"
    FORCE)
set (CMAKE_CXX_FLAGS_SELFCONSISTENCY
    "-g -O0 -DDEBUG_SELFCONSISTENCY"
    CACHE STRING "Flags used by the C++ compiler to build a binary with self-consistency checks during cost updates"
    FORCE)

# ThreadSanitizer
set(CMAKE_C_FLAGS_TSAN
    "-fsanitize=thread -g -O1"
    CACHE STRING "Flags used by the C compiler during ThreadSanitizer builds."
    FORCE)
set(CMAKE_CXX_FLAGS_TSAN
    "-fsanitize=thread -g -O1"
    CACHE STRING "Flags used by the C++ compiler during ThreadSanitizer builds."
    FORCE)

# AddressSanitize
set(CMAKE_C_FLAGS_ASAN
    "-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
    CACHE STRING "Flags used by the C compiler during AddressSanitizer builds."
    FORCE)
set(CMAKE_CXX_FLAGS_ASAN
    "-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
    CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds."
    FORCE)

# LeakSanitizer
set(CMAKE_C_FLAGS_LSAN
    "-fsanitize=leak -fno-omit-frame-pointer -g -O1"
    CACHE STRING "Flags used by the C compiler during LeakSanitizer builds."
    FORCE)
set(CMAKE_CXX_FLAGS_LSAN
    "-fsanitize=leak -fno-omit-frame-pointer -g -O1"
    CACHE STRING "Flags used by the C++ compiler during LeakSanitizer builds."
    FORCE)

# MemorySanitizer
set(CMAKE_C_FLAGS_MSAN
    "-fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2"
    CACHE STRING "Flags used by the C compiler during MemorySanitizer builds."
    FORCE)
set(CMAKE_CXX_FLAGS_MSAN
    "-fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2"
    CACHE STRING "Flags used by the C++ compiler during MemorySanitizer builds."
    FORCE)

# UndefinedBehaviour
set(CMAKE_C_FLAGS_UBSAN
    "-fsanitize=undefined"
    CACHE STRING "Flags used by the C compiler during UndefinedBehaviourSanitizer builds."
    FORCE)
set(CMAKE_CXX_FLAGS_UBSAN
    "-fsanitize=undefined"
    CACHE STRING "Flags used by the C++ compiler during UndefinedBehaviourSanitizer builds."
    FORCE)

# Profiling
set(CMAKE_C_FLAGS_GPROF
  "-pg -DNDEBUG -O3 -Ofast -march=native -mtune=native -funroll-loops"
    CACHE STRING "Flags used by the C compiler during GprofProfiling builds."
    FORCE)
set(CMAKE_CXX_FLAGS_GPROF
  "-pg -DNDEBUG -O3 -Ofast -march=native -mtune=native -funroll-loops"
    CACHE STRING "Flags used by the C compiler during GprofProfiling builds."
    FORCE)
set(CMAKE_C_COMPILER_GPROF
    "/usr/bin/gcc"
    FORCE)
set(CMAKE_CXX_COMPILER_GPROF
    "/usr/bin/g++"
    FORCE)
