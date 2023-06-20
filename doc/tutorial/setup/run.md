---
title: First Simulation
description: This tutorial shows how to run your first simulation with qiotoolkit.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.tutorial.setup.run
---

First Simulation
================

The first simulation we are going to simulate a simple Ising model.
For this we need 2 inputs:

  * The input parameters for the solver we want to run.
  * The description of the Ising cost function ("input data").

A set of examples for these can be found in the `cpp/examples/` directory:


Input Params
------------

[!code-json[Params](../../../cpp/examples/params.sa.json)]


Input Data
----------

[!code-json[Input Data](../../../cpp/examples/input-data.ising.json)]


Running the Solver
------------------

Additionally, the CLI command must specify the solver to use:

```bash
$ cd path/to/qiotoolkit/cpp
$ mkdir release_build
$ cd release_build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make -j8
$ ./app/qiotoolkit --user --log_level=WARN \
      --solver microsoft.simulatedannealing.qiotoolkit \
      --parameters ../data/params1.json \
      --input ../data/ising1.json
```

Runtime Output
--------------

```
[INFO] app/runner.cc:41: Reading file: ../data/params1.json
[INFO] app/qiotoolkit.cc:191: Running solver microsoft.simulatedannealing.qiotoolkit
[DEBUG] solver/simulated_annealing.h:116: recorded cost:10.000000
[DEBUG] solver/simulated_annealing.h:117: calculated_cost:10.000000
[INFO] solver/stepping_solver.h:228: Stop due to step limit: 999, steps:999
[DEBUG] solver/simulated_annealing.h:116: recorded cost:-10.000000
[DEBUG] solver/simulated_annealing.h:117: calculated_cost:-10.000000
[INFO] app/qiotoolkit.cc:201: Writing Response to stdout
```

Results
-------

[!code-json[Results](../../../data/results/ising1_out.json)]

Arguments
--------------
- `--solver` or `-s` (required): The name of the qiotoolkit solver target.
- `--parameters` or `-p` (required): The parameter file containing the input parameters for how the solver should be run.
- `--input` or `-i` (required): The data file containing the input data that specifies the problem.
- `--output` or `-o` (optional): The data file to which the result will be written. If this is not provided, the results will be written to stdout.
- `--user` (optional): Render output in human-readable form.
- `-n` (optional): No benchmarking output is displayed when this flag is set.
- `-v` (optional): Shows the qiotoolkit build version info.

