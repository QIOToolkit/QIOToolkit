---
title: Traveling Salesman Problem
description: This document describes the input format for the tsp model.
topic: article
uid: qiotoolkit.spec.model.tsp
---

Travelling Salesman Problem (TSP)
=================================

```math
\mathcal{H} = \sum_{j\in\Pi_i} \text{dist}(j, j+1)\,,
```

where $`\Pi_i`$ is a permutation of the numbers $`\{0,\ldots,N-1\}`$ and
$`\text{dist}(j, j+1)`$ is the distance between a consecutive pair of nodes in the
parmutation (we equate the nodes `0` and `N`, i.e., the "route" forms a
loop).




Example InputData
-------------------

```json
{
  "type": "tsp",
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

