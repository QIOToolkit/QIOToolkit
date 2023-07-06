---
title: Multi-threading
description: This tutorial explains how to control multi-threading.
topic: article
uid: qiotoolkit.tutorial.usage.multithread
---

Multi-threading
===============

> [!WARNING]
> This feature is currently being reworked. Its parameters and
> functionality are subject to change.

By default, solvers will try to use multiple threads to speed
up the simulation (using OpenMP). You can override this behavior
by setting a maximum number of threads in the `input_params`:

```json
{
  "target": "paralleltempering.qiotoolkit",
  "input_params": {
    "threads": 1,
    ...
  }
}
```

In any case, you can see the number of threads used denoted in
the `benchmark` section of the output:

```json
{
  "benchmark": {
    ...
    "threads": 1,
    ...
  },
  "solutions": [...]
}
```
If the "threads" is larger than the number of CPU cores on the machine or the value is not positive, the parameter "threads" will be ignored.
