---
title: Population Annealing
description: This document contains the I/O specification for population annealing
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.spec.solver.population-annealing
---

Population Annealing
====================

Simulates a population of metropolis walkers with birth-death process
resampling. The qiotoolkit implementation uses variable setpping in temperatures
according to one of 4 available resampling strategies:

  * `linear_schedule`: Change the temperature according to a fixed schedule (non-adaptive).
  * `friction_tensor`: Estimate the next temperature step such that the friction
    tensor remains stable with a constant of `friction_tensor_constant`
  * `energy_variance`: Adjust culling such that the energy variance remains
    stable, using `initial_culling_constant` for the first step.
  * `constant_culling`: Adjust stepping such that a constant portion
    `culling_fraction` of the population is replaced each resampling step.

The initial number of replicas can be specified via `initial_population_size`.
The solver will double the population size whenever

   * There is no remaining cost variance (all replicas are in an equal-cost state)
   * The ratio of families remaining in the population drops below `alpha`
     (indicating that one family dominates the population).

Additionally, you may increase the number of sweeps between resamplings using
the `sweeps_per_replica` parameter.


Example
-------

```json
{
  "target": "microsoft.population-annealing.cpu",
  "version": "1.0",
  "input_params": {
    "seed": 42,
    "population": 100
  },
  "input_data_uri": "..."
}
```


Parameters Specification
------------------------

# [List](#tab/tabid-1)

| param                      | type    | required/default | description    |
| -------------------------- | ------- | ---------------- | -------------- |
| `seed`                     | integer | _time-based_     |                |
| `population`               | integer _>1_  | number of threads | population size. |
| `constant_population`      | bool    | false | whether to keep the population constant between restarts |
| `resampling_strategy`      | string  | `linear_schedule` | resample by `linear_schedule`, `friction_tensor`, `energy_variance` or `constant_culling`. | 
| `beta`                     | Schedule | linear `0..5` | The schedule by which to anneal the system. |
| `beta_start`                     | float _>0_    | 0.0 | initial beta for the `linear_schedule`. |
| `beta_stop`                     | float _>_`beta_start`    | 5.0 | final beta for the `linear_schedule`. |
| `friction_tensor_constant` | float _>0.0_  | 1.0 | friction tensor constant (*required* for `friction_tensor` |
| `initial_culling_fraction` | float _[0,1]_ | 0.5 | initial culling rate (for `energy_variance`) |
| `culling_fraction`         | float _[0,1]_ | 0.2 | constant culling rate (for `constant_culling`) |
| `alpha`                    | float _>1.0_  | 2.0 | ratio to trigger a restart |

# [Schema](#tab/tabid-2)

[!code-json[Schema](population-annealing.schema)]

***
