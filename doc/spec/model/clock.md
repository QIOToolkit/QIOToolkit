---
title: Clock model
description: This document describes the input format for the clock model.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.spec.model.clock
---

Clock Spin Glass
================

The clock model (and corresponding clock spin glass) describes interacting spins
which can take discrete values on the 2D planar unit circle.

```math
\mathcal{H} = -\sum_{ij} J_{ij}\, s_i\!\cdot\!s_j,\quad s_i=(\cos\varphi_i, \sin\varphi_i)
```

where $`s_i\!\cdot\!s_j`$ is a scalar product and each $`\varphi_i`$ can take
one of $`q`$ values:

```math
\varphi_i = v_i \frac{2\pi}{q},\quad v_i\in\{0,\ldots,q-1\}
```

Or, as a visual representation for $`q=5`$:

\begin{tikzpicture}
  \draw (0,0) circle(2);
  \draw[->,line width=1.5pt] (0,0) -- (30:2);
  \draw[->,line width=1.5pt] (0,0) -- (102:2);
  \draw[->,line width=1.5pt] (0,0) -- (174:2);
  \draw[->,line width=1.5pt] (0,0) -- (246:2);
  \draw[->,line width=1.5pt] (0,0) -- (318:2);
\end{tikzpicture}

*NOTE*: For the case discretized to two values, $`q=2`$, this model corresponds
to the [Ising spin glass](ising.md).

Hypergraph Generalization
-------------------------
The scalar product in the standard clock Hamiltonian does not lend itself to a
generalization for multi-spin interactions. Instead, the `qiotoolkit` implementation
uses a Hamiltonian based on the normalized sum of vectors $`\vec v_e`$:

```math
\mathcal{H} = -\sum_e J_e (2v_e^2-1),\quad
\vec v_e = \frac{1}{|e|}\sum_{i\in e} \vec{s_i}\,
```

where the term in brackets reduces to the scalar product for the case of 2-spin
interactions and covers the range $`[-1,1]`$ for any $`q`$:
  
  * The minimal value is realized when the clock spins "cancel out" (i.e., sum
    up to $`\vec 0`$)
  * The maximal value is realized when all spins are aligned (have the same value).

Example Input Data
------------------

```json
{
  "type": "clock",
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

