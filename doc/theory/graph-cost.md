---
title: Graph Correspondency
description: This document outlines the graph interpretation of cost terms.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.theory.graph-cost
---

Graph Cost Functions
====================

For models with a polynomial cost function of the form

```math
\mathcal{H} = \sum_i c_i \prod_{j\in i} s_j
```

such as the Ising or PUBO models, we define the following
hyper-graph correspondency:

  * Each variable of the model is associated with a node
    of the graph.
  * Each term in the cost function gives rise to an interaction
    between the variables in this term. This is interpreted as
    a "hyper-edge" connecting these nodes.
  * The interaction constant $`c_i`$ is taken to be the weight
    of this edge.

> [!NOTE]
> For the QUBO model (or an Ising model where no term has more
> then two variables interacting), this correspondency resolves
> to a regular graph where nodes are connected in pairs.

As such we can describe each cost function of the above form
as a hyper-graph.

Implementation
--------------

qiotoolkit provies a [model::GraphModel](../api/model/graph-model.yml) base class
which implements the input and access of a graph in the edge-list
representation used by the QIO solvers in Azure quantum. Namely:

```json
"terms": [
  {"c": 2, "ids": [0,1,2]},
  {"c": -3, "ids": [3,4,5]}
  ...
]
```

Where the listed terms describe

  * An edge with weight $`+2`$ connecting spins 0..2, and
  * An edge with weight $`-3`$ connecting spins 3,4,5

This would result in a cost function of the form

```math
\mathcal{H} = +2s_0s_1s_2 -3s_3s_4s_5 + \cdots
```

The corresponding interfaces to access these `nodes()` and `edges()` can
be found in the [API section](../api/model/graph-model.yml).
