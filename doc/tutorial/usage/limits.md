---
title: Limits
description: This tutorial shows how to set simulation limits.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.tutorial.usage.limits
---

Make it Stop
============

There are several options to define how long your simulation should
run.

Limit Parameters
----------------

You should denote at least one of the following stopping conditions. If multiple are given, qiotoolkit will stop when the first one is reached.

| name | description |
| ---- | ----------- |
| `limit_time` | How many **wall-clock seconds** the simulation should run. qiotoolkit aims for a 1-s heartbeat (can be changed using `tick_every`) and will adjust the steps aiming to stop just before the `limit_time` is reached. _This does not consider time spend in multiple vs single threads_. |
| `step_limit` | How many **steps** the solver should take. The final step denoted is inclusive. |
| `eval_limit` | How many **times the cost function should be evaluated**. qiotoolkit will stop at the end of the step during which the `eval_limit` is surpassed. _This treats full and partial cost function evaluations equivalently_. |
| `cost_limit` | Stop when a cost **equal or lower to** `cost_limit` is reached. Will stop at the end of the step during which the requested cost is matched. _This allows benchmarking time-to-solution_.
| `timeout`    | **Alias of** `time_limit` for QIO compatibility |
| `sweeps`     | **Alias of** `step_limit` for QIO compatibility |

Signals
-------

While the process is running, you can send the following signals to the process:

| signal | effect |
| ------ | ------ |
| `SIG_USR1` | show current status on `stderr` |
| `SIG_USR2` | stop at the end of the current step |
| `SIG_TERM` | terminate immediately |
| `SIG_INT` <br/> (`Ctrl-C`) | 1st: show status,<br/> 2nd: stop after step, <br/>3rd: terminate immediately |

> [!NOTE]
> To trigger the 2nd and 3rd option of `SIG_INT`, signals must be received
> within 2.0 wall clock seconds from each other (otherwise the counter resets
> to 0).




