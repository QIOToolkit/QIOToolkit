---
title: Simulated Annealing
description: This document contains the I/O specification for simulated annealing
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.spec.solver.simulated-annealing
---

Simulated Annealing
===================

Performs metropolis dynamics on the model while gratually lowering ("annealing")
the simulation temperature. As a result, the system settles in a low-cost state
while still being allowed to escape local minima as the simulation progresses.

  * qiotoolkit's simulated annealing implementation allows the specification of an
    annealing schedule either as an explicit array of values or as a
    schedule-generator.

  * This (explicit or schedule-generated) list of numbers can be interpreted as
    inverse temperatures by specifying `use_inverse_temperature` = `true`.

  * Multiple restarts can be performed using the `restarts` parameter. These
    restarts are mutually independent.

> [!NOTE]
> If you use the Azure QIO-comptaible parameters `beta_start` and `beta_stop`,
> the solver will attempt to infer the number of annealing steps from `step_limit`
> or its Azure QIO-alias `sweeps`.


Example
-------

```json
{
  "target": "microsoft.simulated-annealing.qiotoolkit",
  "version": "1.0",
  "input_params": {
    "seed": 42,
    "schedule": {
      "type": "geometric",
      "initial_value": 3.0,
      "final_value": 0.2,
      "count": 128
    }
    "restarts": 8
  },
  "input_data_uri": "..."
}
```

This will perform 8 restarts of cooling the system from T=3.0 to T=0.2 in 128 steps
spaced geometrically.


Parameters Specification
------------------------

# [List](#tab/tabid-1)

| param   | type | required/default | description |
| ------- | ---- | ---------------- | ----------- |
| `seed`  | integer | default: _time-based_ |  |
| `schedule` | [Schedule]() required | How the temperature should be changed over time. |
| `use_inverse_temperature` | boolean | default: false | Whether to interpret `schedule` as inverse temperature. |
| `beta_start` | float _>0_ | (QIO compatibility) | Float specifying the starting inverse temperature. |
| `beta_stop` | float _>beta_start_ | (QIO compatibility) | Float specifying the stopping inverse temperature. |
| `restarts` | integer > 0 | default: 1 | How many restarts to perform. |

# [Schema](#tab/tabid-2)

[!code-json[Schema](simulated-annealing.schema)]

***
