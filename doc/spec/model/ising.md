---
title: Ising model
description: This document describes the input format for the ising model.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.spec.model.ising
---

Ising Spin Glass
================

The Ising spin glass Hamiltonian has the form:

```math
\mathcal{H} = \sum_{ij} c_{ij} s_i s_j\,,
```

where the sum is over interacting pairs $`ij`$ and the spins take the values
$`s_i\in\{\pm1\}`$. The $`c_{ij}`$'s are interaction constants which are
specified as part of the problem description.

> [!CAUTION]
> This definition differs from the canonical version used in statistical
> mechanics by a global minus sign! As a result, positive interaction
> constants $`c_{ij}\gt0`$ give rise to **anti-ferromagnetic** interaction.

Graph Interpretation
--------------------

We consider each of the spins $`s_i`$ to reside on the node $`i`$ of a
[weighted graph](../../theory/graph-cost.md) and the interactions
$`c_{ij}`$ to represent edge weights between the respective nodes $`i`$ and
$`j`$. For instance, the Hamiltonian

```math
\mathcal{H}(\vec s) = - (s_0s_1 + s_1s_2 + s_2s_3 - 2s_3s_0)
```

would correspond to the graph

\begin{tikzpicture}
  \draw[line width=1pt] (0,3) -- (3,3) -- (3,0) -- (0,0);
  \draw[line width=3pt,color=red] (0,0) -- (0,3);
  \draw[fill=white] (0,3) circle(0.5);
  \draw[fill=white] (3,3) circle(0.5);
  \draw[fill=white] (3,0) circle(0.5);
  \draw[fill=white] (0,0) circle(0.5);
  \node at (0,3) {$0$};
  \node at (3,3) {$1$};
  \node at (3,0) {$2$};
  \node at (0,0) {$3$};
  \node[color=red] at (0.5,1.5) {$-2$};
  \node at (1.5,2.5) {$+1$};
  \node at (2.5,1.5) {$+1$};
  \node at (1.5,0.5) {$+1$};
\end{tikzpicture}


Hypergraph Generalization
-------------------------
The above interpretation holds for Hamiltonians with terms consisting of
exactly two spins. For optimization problems, we are interested in a more
general Hamiltonian of the form

```math
\mathcal{H} = -\sum_e c_e \prod_{i\in e} s_i
```

where the sum is over all edges $`e`$ in the list of `terms` and the product
is over the `ids` $`i`$ participating in this term. In this case, any number
of spins can interact in a given term -- akin to a "hyper edge" in the graph.

Example Input Data
------------------

A configuration for the general Ising spin glass Hamiltonian must

  * carry the `label "ising"` and corresponding `version "1.0"`
  * specify a non-empty array of `terms` in the Hamiltonian (or, equally, the
    hyper edges of the graph):
    * each edge must have a cost `c` (denoted $`c_e`$ in the Hamiltonian).
    * each edge must have an array `ids` of the nodes participating in the
      interaction.
    * the `ids` in each term must be unique (no repetition within a given term)
    * the `ids` mentioned in all terms must form a consecutive set `[0..N-1]`.

**Example:**

```json
{
  "type": "ising",
  "version": "1.0",
  "terms": [
    {"c": 1.0, "ids": [0, 1]},
    {"c": 1.0, "ids": [1, 2]},
    {"c": 1.0, "ids": [2, 3]},
    {"c": -2.0, "ids": [3, 0]}
  ]
}
```

*NOTES*:

  * The list of `ids` can be empty, which is interpreted as a constant term in
    the Hamiltonian.
  * If there is only a single number in `ids`, this corresponds to a local
    field.
  * Repetitions are not allowed since any odd number is equivalent to a single
    occurence (and any even number can be dropped).
  * The number of spins $`N`$ is inferred from the highest number found in
    `ids` of any term (which is presumed to represent $`N-1`$).
