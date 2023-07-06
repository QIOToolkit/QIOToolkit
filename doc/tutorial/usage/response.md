---
title: Anatomy of a Response
description: This tutorial outlines the anatomy of a response
topic: article
uid: qiotoolkit.tutorial.usage.response
---

Anatomy of a Response
=====================

A qiotoolkit response is in JSON format. It contains:

  * The configuration (both explicit and inferred parameters)
  * Telemetry including basic benchmarking attributes
    and qiotoolkit/solver-specific extensions
  * The best solution(s) found

```json
{
  "target": "parallel-tempering.qiotoolkit",
  "input_params": {...}
  "input_data": {...}
  "benchmark": {
    "execution_time_ms": 19.98242,
    ...
    "extensions": [...]
  },
  "solutions": [
  ]
}
```

## Configuration


## Telemetry

The `benchmark` field contains basic benchmarking measures as defined [in
conjunction with the QIO service solvers](../../spec/response/benchmark.md). This
denotes the resources consumed by the simulation (wall time, threads, memory
usage, cost function evaluations).

## Benchmark Extensions

Additionally, qiotoolkit will populate a number of extensions to this shared format
to indicate provenance metadata and solver properties.

### Build information

The build information extension ensures data provenance w.r.t. the version of
the code which was used to generate a specific response. It contains the build
type, branch and git hash of the code that was compiled.

```json
  "extensions": [
    ...
    {
      "name": "build",
      "value" {
        "build_type": "RELEASE",
        "branch": "master",
        "hash": "a892de1"
      }
    },
    ...
  ]
```

> [!NOTE]
> Ensure you always **manually enable** the RELEASE build type when running
> large scale simulations. There is a notable overhead for running in DEBUG
> mode (default).

### Solver properties

The `AbstractSolver` base class inherits from `utils::Observer`, allowing it
to take measurements during the course of the simulation. These measurements
are summarized in the `solver` extension.

## Solutions

Solutions will always be an array containing one or multiple solutions found
by the solver. As these are stochastic methods, there is no guarantee that
they are optimal.
