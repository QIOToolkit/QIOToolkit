---
title: Potts Model
description: This document describes the input format for the Potts model.
topic: article
uid: qiotoolkit.spec.model.potts
---

Potts Spin Glass
================

The Potts Hamiltonian has the form

```math
\mathcal{H} = -\sum_{ij} \delta_{s_i,s_j}\,,
```

where the sum is over interacting spin pairs $`i`$, $`j`$ and $`\delta`$ is the
Kronecker delta:

```math
\delta_{a,b} = \begin{cases}
  1 & a=b\\
  0 & a\neq b
\end{cases}
```

Example Input Data
------------------

```json
{
  "type": "potts",
  "version": "0.1",
  "q": 5,
  "terms": [
    {"c": 1.0, "ids": [0, 1]},
    {"c": 1.0, "ids": [1, 2]},
    {"c": 1.0, "ids": [2, 3]},
    {"c": -2.0, "ids": [3, 0]}
  ]
}
```

