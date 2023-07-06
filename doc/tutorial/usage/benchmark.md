---
title: Benchmark
description: This tutorial shows how to benchmark a solver.
topic: article
uid: qiotoolkit.tutorial.usage.benchmark
---

Benchmark
=========

> [!CAUTION]
> Always ensure you are using a binary [built with](../setup/cmake.md) a
> `Release` configuration; there is a substantial difference in performance.
> If in doubt, you can check the `build` extension of the `response.benchmark`.

Usage
-----

Details on how to run qiotoolkit can be found [here](../setup/run.md). To display
the benchmarking logging data, ensure that you run the qiotoolkit executable without
the `-n` flag:

```bash
$ cd path/to/qiotoolkit/cpp
$ mkdir release_build
$ cd release_build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make -j8
$ ./app/qiotoolkit --user --log_level=WARN --solver simulatedannealing.qiotoolkit \
      --parameters ../examples/params-sa.json \
      --input ../examples/ising.json
```


Wall time of a run
------------------

The wall clock duration of a simulation can be gathered from the
`benchmark.end2end_time_ms` in the [qiotoolkit Response](response.md).  For this to
be meaningful, the simulation should be run with a `step_limit` or
(alternatively) `eval_limit`. _When a `timeout` is set and reached, this
will only show how precise qiotoolkit's stop timing was..._

> [!NOTE]
> For [multi-threaded](multithread.md) simulations, you can also compare this
> value to `brenchmark.end2end_cputime_ms` to evaluate how efficient the
> multi-threading was. Ideally the latter should approach `threads` times the
> former.

Memory Usage
------------

Memory usage is returned as part of the response in the field
`benchmark.max_memory_usage_bytes`.

Time to Solution
----------------

By setting a `params.cost_limit` you can tell qiotoolkit to [stop the simulation](limits.md)
once the expected lowest energy (or an even lower one...) is found. Use this limiting
condition in conjunction with the wall clock duration mentioned above, keeping an eye
on the number of `benchmark.threads` used when comparing results.
