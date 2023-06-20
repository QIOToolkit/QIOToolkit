---
title: API documentation
description: This document gives an overview of the API documentation.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.api
---

API documentation
=================

This section of the qiotoolkit documentation contains auto-generated
C++ interface-descriptions for the components in the repository.

  * For the input and output specifications of the compiled binary,
    check out the [Specifications Section](/spec)

  * For a step-by-step guide on getting started with qiotoolkit, please
    refer to the [Tutorial Section](/tutorial)


Models
------

All qiotoolkit models are expected to inherit from the
[markov::Model](markov/model.yml) base class, which defines the required methods
to interface with solvers. Currently, the code base contains implementations
for:

  * [Ising Model](model/ising.yml)
  * [PUBO](model/pubo.yml)
  * [Blume-Capel](model/blume-capel.yml) (_Spin-1, i.e., 3 states_)
  * [Potts](model/potts.yml) (_p-state, no notion of "neighboring values"_)
  * [Clock](model/clock.yml) (_p-state, neighboring values + periodic boundaries_)
  * [TSP](model/tsp.yml) (_single-tour on fully connected graph_)
  * [Poly](model/poly.yml) (_**Experimental**: Arbitrary polynomial cost function with parameters_)

Solvers
-------

All qiotoolkit solvers are expected to inherit from the
[solver::AbstractSolver](solver/abstract-solver.yml) base class, which defines the
interface to the simulation runner.

Current built-in solvers include:

  * [Simulated Annealing](solver/simulated-annealing.yml)
  * [Parallel Tempering](solver/parallel-tempering.yml)
  * [Population Annealing](solver/population-annealing.yml)
  * [Substochastic Monte-Carlo](solver/substochastic-monte-carlo.yml)
  * [MUlti-objective Replica Exchange](solver/murex.yml) (_**Experimental**_)

Utilities
---------

  * `Utils` - general purpose utilties for I/O and debugging
  * `Matcher` - Specification of pre-conditions for valid input
  * `Graph` - Graph representation
  * `Schedule` - Generators for annealing sequences and temperature sets
  * `Observe` - Built-in measurement functionality
