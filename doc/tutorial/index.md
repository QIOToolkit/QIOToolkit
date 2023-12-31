---
title: Tutorials
description: This document lists the available tutorials for qiotoolkit.
topic: article
uid: qiotoolkit.tutorial
---

Tutorials
=========


Getting Started
--------------

If this is your first time working with the qiotoolkit , here's a good
place to start; build the binary and run your first simulation:

 * [Build the QIOToolkit](setup/build.md)
 * [Run your first simulation](setup/run.md)

Usage
---------

Once you have a running binary (see previous section), this section
explains how to run configure a request to run a specific solver on
a particular model:

 * [Anatomy of a request](usage/request.md)
 * [Make it stop!](usage/limits.md)
 * [Choosing a solver](usage/solvers.md)
 * [Choosing a model](usage/models.md)
 * [Multithreading](usage/multithread.md)
 * [Anatomy of a response](usage/response.md)
 * [Benchmarking](usage/benchmark.md)

Development
-----------

Beyond the built-in functionality, you may also opt to modify a model to use
custom simulation dynamics (types of steps), create a new model or even roll
your own solver. Besides these tutorials, the [API](../api/index.md)
section provides machine-generated interface descriptions.

* [Configuration](dev/configuration.md)
* [Logging from qiotoolkit](dev/log.md)
* [Writing a custom model](dev/model.md)
* [Adding a solver](dev/solver.md)
