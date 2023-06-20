---
title: Anatomy of a Request
description: This tutorial outlines the anatomy of a request
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.tutorial.usage.request
---

Anatomy of a Request
====================

The default qiotoolkit input format is JSON, although it would be possible to handle
other formats with different input handlers.

A minimal request for qiotoolkit has the following shape:

```json
{
  "target": "SOLVER_IDENTIFIER",
  "input_params": {
     ...
  }
}
```

```json
  "cost_function": {
    "type": "MODEL_IDENTIFIER",
    "version": "MODEL_VERSION",
    ...
  }
}
```

Where the `SOLVER_IDENTIFIER` must exactly match the
value returned by `AbstractSolver::get_identifier()`.

Likewise, `MODEL_IDENTIFIER` and `MODEL_VERSION` must match those 
return by `markov::Model::get_identifier()` and `markov::Model::get_version()`.

> [!NOTE]
> These identifiers and version numbers not only tell qiotoolkit what to simulate
> but ensure there is no mismatch in versioning (i.e., you're trying to run old
> input with a substantially modified newer version of the code or vice-versa.

## Input Parameters

Additional configuration for the solver should be denoted in the `input_params`
section of the request. While some parameters are shared between all solvers
(e.g., `time_limit`), others are specific to the solver. For instance, the
Parallel Tempering solver can be configured with:

```json
  ...
  "input_params": {
    "max_steps": 10,
    "seed": 42,
    "temperature_set": {
      "type": "geometric",
      "low": 0.5,
      "high": 3,
      "steps": 32
    }
  },
  ...
```

> [!NOTE]
> For the exact set of parameters understood by a solver, refer to the [solver
> specifications](../spec/request/solver/index.md).

## Input Data

The model is configured from additional parameters in the `input_data` section.
These input parameters determine which cost function will be optimized. For
instance, the `Ising` model expects a `term` entry describing the terms of the
polynomial in the Ising cost function:

```json
  ...
  "input_data": {
    "type": "ising",
    "version": "1.0",
    "terms": [
      {"c": -1, "ids": [0,1]},
      {"c": 2, "ids": [0,2]},
      {"c": 1, "ids": [1,2]},
      ...
    ]
  },
  ...
```

Which, in this case, describes a cost function of the form

```math
  \mathcal{H} = -s_0s_1 + 2s_0s_2 + s_1s_2 + \cdots
```

> [!NOTE]
> For the exact set of parameters required by each model, refer to the [model
> specifications](../spec/request/model/index.md).
