---
title: Model Development
description: This explains how to implement a new model in qiotoolkit.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.dev.model
---

Model Development
=================

A new model is created by inheriting from the `markov::Model` class.
This base class defines the virtual pure methods which the new model
must implement:

| method | purpose |
| ------ | ------- |
| `get_identifier()` | string identifying the model -- this will be the `type='...'` value in the input. |
| `get_version()`    | string identifying the version -- this will be the `version='...'` value in the input. |
| `configure(...)`   | method telling qiotoolkit how to read model configuration from input. |
| `calculate_cost(state)` | Implements the (parametrized) cost function |
| `calculate_cost_difference(state, transition)` | Calculates how much the cost will change if transition is applied. |
| `get_random_state(rng)` | Creates a new random starting `state` |
| `get_random_transition(state, rng)` | Proposes a random change to the `state` |
| `apply_transition(transition, state)` | Modifies the `state` as dictated by the `transition` |

These methods entail the essence of the model: They define the state space and
how one can move within it, as well as the association of a cost with each
sate. The dynamics of whether to accept or reject a transition are left to the
user.

Self consistency
----------------

The implementation of `calculate_cost_difference()` and `calculate_cost()` must
be consistent. That is,

```math
\text{costdiff}(\text{before}, \text{transition}) \equiv \text{cost}(\text{transition}(\text{before})) - \text{cost}(\text{before})
```

You can use the [SelfConsistency Build](../setup/cmake.md) to verify this holds.

> [!NOTE]
> `calculate_cost_difference()` **could** be based on a call to
> `calculate_cost()` with a modified state, but this is less efficient for
> models where individual changes affect only a small number of terms.

State and Transition
--------------------

In some situations it is sufficient to use a base type to represent the transition
(or even the state); in others you need to define your own class to hold this
information. You may extend `markov::State` and `markov::Transition`, respectively,
to get the right interfaces. Whichever route you opt for, you need to **template**
the base class of your new model with this two pieces of information:

```cpp
class MyModel : public ::markov::Model<MyState, MyTransition> {
 public:
  using State_T = MyState;
  using Transition_T = MyTransition;
  ...
};
```

> [!NOTE]
> I find it convenient to typedef these two to `State_T` and `Transition_T` to
> homogenize the interfaces which need to be overloaded.

Graph Model
-----------

The special base class `::model::GraphModel` makes use of the
[Graph-Cost](../../theory/graph-cost.md) correspondency and provides
the appropriate interfaces to access the graph structure. If your cost function
has the appropriate shape, this can simplify writing the cost function logic
substantially.

Example
-------

The following is an example implementation of the above interfaces for a soft-spin
Model. It can be found in the `cpp/examples/` directoy of the qiotoolkit codebase.

[!code-cpp[Soft-Spin model](../../../cpp/examples/soft_spin.h)]
