---
title: Parallel Tempering
description: This document contains the I/O specification for parallel tempering.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.spec.solver.parallel-tempering
---

Parallel Tempering
==================

qiotoolkit's parallel tempering implementation does not currently adjust the set of
temperatures automatically to the model (upcoming feature). As of now, the set
of `temperatures` needs to be specified as an input parameter.

  * For this you can either use an explicit array or a schedule generator.
  * Alternatively, you may specify the set of inverse temperatures via `all_betas`.
    

Example
-------

```json
{
  "target": "microsoft.parallel-tempering.qiotoolkit",
  "version": "1.0",
  "input_params": {
    "seed": 42,
    "temperatures": {
      "type": "geometric",
      "initial_value": 3.0,
      "final_value": 0.2,
      "count": 32
    }
  },
  "input_data_uri": "..."
}
```

This example uses a generator to create a geometric series of temperatures
ranging from `3.0` to `0.2` with 32 steps. This means that the simulation will
use 32 replicas spaced such that 

```math
a_i = c*a_{i+1}
```

with a constant `c` selected to match the initial and final value requested.

Parameters Specification
------------------------

# [List](#tab/tabid-1)

| param   | required/default | description |
| ------- | ---------------- | ----------- |
| `seed`  | default: _time-based_ |
| `temperatures` | required | How the temperature should be changed over time. |
| `all_betas` | required | How the inverse temperature should be changed over time. |

# [Schema](#tab/tabid-2)

[!code-json[Schema](parallel-tempering.schema)]

***
