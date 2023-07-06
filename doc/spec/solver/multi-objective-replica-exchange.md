---
title: Multi-Objective Replica Exchange
description: This document contains the I/O specification for Multi-Objective Replica Exchange.
topic: article
uid: qiotoolkit.spec.solver.multi-objective-replica-exchange
---

Multiobjective Replica Exchange (MUREX)
=======================================

> [!CAUTION]
> This solver is experimental; it is still subject to development which may
> alter its API and performance attributes.

MUREX simulates replicas at different temperature **AND** parameter values
concurrently. This is currently only supported by the [Poly](../model/poly.md)
model which has parameters which can be adjusted during the simulation.

This is achieved by defining a graph of nodes, each associated with different
parameter values and a temperature. Replica exchanges are performed between
nearby nodes according to the graph's edges.

> [!NOTE]
> The current implementation has a grid arrangement of nodes hard-coded into
> the setup routine. This will be adjusted to be more configurable in upcoming
> versions.
