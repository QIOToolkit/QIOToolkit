=qiotoolkit

This project contains components for building and running Quantum inspired algorithms to solve various types of optimization problems that can be expressed as unbounded binary optimization problems.  

==Repo Organization

The contents of this repository are separated into the following components:

* **[utils](utils/README.md)**: utils library of shared functionality:
  logging, configuration, debugging and random number generation.

* **[graph](graph/README.md)**: Hypergraph representation.

* **[markov](markov/README.md)**: Markov process representation.

* **[model](model/README.md)**: Base classes for optimization problems.

* **[solver](solver/README.md)**: Base implementation of solvers and
  heuristic search.

==Install Boost
sudo apt-get install -y libboost-all-dev=1.65.1.0ubuntu1

==Build Dev Evironment
For Linux
	run setup/linuxsetup.sh

==Build System

Build files are generated with (https://cmake.org/)[CMake]. For a simple
**debug_build** and test, follow these steps in a checked out repository:

  ~ $ cd /path/to/repo
  /path/to/repo $ mkdir debug_build && cd debug_build
  /path/to/repo/debug_build $ cmake -DCMAKE_BUILD_TYPE=Debug ..
  /path/to/repo/debug_build $ make
  /path/to/repo/debug_build $ make test

To compile a **release** version (essential for large scale simulations), use

  ~ $ cd /path/to/repo
  /path/to/repo $ mkdir release_build && cd release_build
  /path/to/repo/release_build $ cmake -DCMAKE_BUILD_TYPE=Release ..
  /path/to/repo/release_build $ make

==Build Profiling
In order to build your code with profiling code, you need to define build macro "qiotoolkit_PROFILING" to
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -std=c++14 -fPIC -fopenmp -Wno-unknown-pragmas -pedantic")
like:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -std=c++14 -fPIC -fopenmp -Wno-unknown-pragmas -pedantic -Dqiotoolkit_PROFILING")
in CMakeSettings.json under cpp directory.

