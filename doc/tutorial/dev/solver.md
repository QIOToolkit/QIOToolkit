---
title: Solver Development
description: This explains how to implement a new solver in qiotoolkit.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.dev.solver
---

Solver Development
==================

A new solver is created by extending `solver::AbstractSolver` or `solver::SteppingSolver`.
The latter has stepping logic already implemented and is currently used for all the
built-in solvers. It is customary to template your solver for `Model_T` -- the
class of the model to be simulated.

The base class defines which methods must be implemented for the solver to work.
For a `SteppingSolver`, they are:

| method | purpose |
| ------ | ------- |
| `get_identifier()` | string identifying the solver -- this will be the `target='...'` value in the input. |
| `configure(...)`   | method telling qiotoolkit how to read model configuration from input. |
| `init()`           | called after `configure()`, allowing you to initialize based on configuration. |
| `make_step(step)`  | perform the next step in the simulation. |
| `get_model_sweep_size() ` | should return the sweep size of the model being simulated\* |
| `get_lowest_cost()`       | should return the lowest cost found so far (used for `limit_cost` condition) |
| `get_solutions()`         | should render the best solution(s) found. |

Implementation
--------------

The solver should rely on the interfaces provided by a `markov::Model` to perform its work. They are sufficient
to build a markov chain, but might not cater to the need of more sophisticated solvers. For instance, the MUREX
solver can currently only simulate the `Poly` model, because it relies on the additional parameter interfaces
it provides.

Example
-------

The following shows an implementation of a "steep descent" algorithm. Its deliberately not called "steepest"
because the `::markov::Model` interfaces do not allow us to enumerate all possible transitions from a given
starting point. (And, depending on the model, this might be infeasible). Instead we settle for sampling a
number of different candidates and picking the best choice (or none) at each step:

[!code-cpp[Descent Solver](../../../cpp/examples/descent.h)]
