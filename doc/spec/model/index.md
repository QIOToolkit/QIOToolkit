---
title: Model Specifications
description: This document describes outlines the model input format.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.spec.model.models
---

Built-in Models
===============

qiotoolkit has the following built-in models:

| Model | Description |
| ---------- | ----------- |
| [ising](../../spec/model/ising.md) | The Ising model consists of binary variables $`\in\{\pm1\}`$ with a cost function that is the sum of weighted variable-products. |
| [pubo](../../spec/model/pubo.md), qubo | The PUBO ("polynomial unconstrained binary optimization") and QUBO ("quadratic ...") models have binary variables $`\in[0,1]`$ and a cost function that sums terms of 2 (qubo) or more variables (pubo). |
| [blume-capel](../../spec/model/blume-capel.md) | The Blume-Capel model extends the Ising model to **Spin-1** (i.e, 3 states, $`[-1,0,1]`$). |
| [potts-model](../../spec/model/potts.md) | Each variable in the Potts model can have one of $`p`$ values $`[0,1,\ldots,p-1]`$ and the cost function compares variables for equality. That is, there is no notion of _neighboring_ values |
| [clock-model](../../spec/model/clock.md) | Each clock variable (or "rotor") can have on of $`p`$ discrete values which can be interpreted as a discretized angle (periodic boundaries). The cost function compares how similarly aligned interacting variables are. |
| [tsp](../../spec/model/tsp.md) | The travelling salesman problem asks to find the shortest tour visiting all nodes in a graph. |
| [poly](../../spec/model/poly.md) | Cost function constructed from nested polynomial terms with mutable parameters. _Experimental_ |

State Spaces
------------

In addition to the built-in models, components for `Partition` and `Permutation` are provided as pre-defined state-spaces.
