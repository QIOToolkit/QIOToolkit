---
title: Substochastic Monte Carlo
description: This document contains the I/O specification for substochastic Monte Carlo.
topic: article
uid: qiotoolkit.spec.solver.substuchastic-monte-carlo
---

Substochastic Monte Carlo
=========================

Simulates a population of **random** walkers with birth-death resampling.
qiotoolkit's implementation allows both the stepping probability `alpha` and the
resampling factor `beta` to be specified as a schedule (over simulation steps).

Additionaly, you may specify the `target_population` and the number of
`steps_per_walker` as input parametes.

Example
-------

```json
{
  "target": "substochastic-monte-carlo.qiotoolkit",
  "version": "1.0",
  "input_params": {
    "seed": 42,
    "alpha": {
      "initial_value": 0.8,
      "final_value": 0.2,
      "count": 1e3
    }
    "beta": {
      "initial_value": 0.2,
      "final_value": 0.8,
      "count": 1e3
    }
    "target_population": 800
  },
  "model": {...}
}
```

This simulates a population of 800 random walkers over the course of 200 steps.
It starts out with an emphasis on stepping (`alpha=0.8`) and gradually changes
to a resampling regime.

> [!NOTE]
> Substochastic Monte-Carlo does currently **NOT** adjust the energy scale of
> your model. Therefore the magnitude of `beta` may need to be adjusted for
> your needs.


Parameters Specification
------------------------

# [List](#tab/tabid-1)

| param   | type | required/default | description |
| ------- | ---- | ---------------- | ----------- |
| `seed`  | integer | default: _time-based_ | |
| `target_population` | integer | **required** | The desired population size (throughout the simulation). |
| `alpha` | [Schedule]() | **required** | How the stepping probability `alpha` should be changed over time. |
| `beta` | [Schedule]() | **required** | How the resampling factor `beta` should be changed over time. |
| `steps_per_walker` | float _>0_ | | Number of steps to attempt for each walker. |

# [Schema](#tab/tabid-2)

[!code-json[Schema](substochastic-monte-carlo.schema)]

***
