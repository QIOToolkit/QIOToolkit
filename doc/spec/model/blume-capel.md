---
title: Blume-Capel model
description: This document describes the input format for the blume-capel model.
topic: article
uid: qiotoolkit.spec.model.blume-capel
---

Blume-Capel Model
=================
The variables in the `BlumeCapel` model can take 3 values
(spin-$`1`$ as opposed to spin-$`\frac12`$ in the Ising case).

```math
\mathcal{H} = -\sum_{ij} J_{ij} s_{i}s_{j}\,,\quad s_i \in \{-1,0,+1\}
```

Hypergraph Generalization
-------------------------
The generalization is analogous to the [Ising spin glass](ising.md):

```math
\mathcal{H} = -\sum_e J_e \prod_{i\in e} s_i
```

*NOTE*: Contrary to the Ising model, squared spin variables can be meaningful in the
`BlumeCapel` model: $`s_i\in\{-1,0,+1\}`$, $`s_i^2\in\{0,1\}`$.


Example Input Data
------------------

```json
{
  "type": "blume-capel",
  "version": "0.2",
  "terms": [
    {"c": 1.0, "ids": [0, 1]},
    {"c": 1.0, "ids": [1, 2]},
    {"c": 1.0, "ids": [2, 3]},
    {"c": -2.0, "ids": [3, 0]}
  ]
}
```
