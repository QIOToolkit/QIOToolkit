---
title: Metropolis Walker
description: This document explains a walker with metropolis dynamics.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.theory.metropolis-walker
---

Metropolis Walker
=================
The Boltzmann weight describes the likelyhood of finding a physical system
in a state $`s`$ at a given temperature $`T`$:

```math
  p(s) \propto e^{-E(s)/k_B T}
```

where $`E(s)`$ is the energy of the system in state $`s`$ (in the context of
optimization, this is often referred to as the "cost function"). The actual
value of $`p(s)`$ could be obtained via normalization:

```math
  p(s) = \frac{e^{-E(s)/k_BT}}{Z},\quad Z = \sum_{s'} e^{-E(s')/k_B T}
```

where the normalizing denominator $`Z`$ is called the "partition function" and
the sum is over all possible states of the system. For configuration spaces
of interesting optimization problems, $`Z`$ is typically intractable (that is,
the number of possible configurations is too large to compute it).

*NOTE*: The lower the temperature $`T`$, the highter the weight-discrepancy
between "low energy" and "high energy" states. In the limit $`T \to 0`$, the
weight of all but the lowest energy state vanishes and we find the system in
this "ground state" with probability $`1`$.

Randomized Sampling
-------------------
The metropolis algorithm describes a markov chain approach for
randomized sampling of a system's states with their sampling probability
converging to the Boltzmann weight. To extend the markov chain, a random next
state is proposed and accepted with probability

```math
  p(s\to s') =
  \begin{cases}
    e^{-(E(s')-E(s))/k_BT},& \text{if } E(s') > E(s)\\
    1,              & \text{otherwise.}
  \end{cases}
```

That is, we always accept a move to a lower-energy state, but only allow
increases depending on the energy difference $`\Delta_E`$ and temperature
$`T`$. This sampling approach can be proven to
converge to the Boltzmann distribution in the
long-term limit.

Implementation
--------------
Metropolis sampling is implemented as a specialization of the
[`Walker`](../api/markov/walker.yml) base class:

    :::cpp linenums=False
    template <class Model>
    class Metropolis : public Walker<Model> {
     public:
      /// Decide whether to accept a given cost increase.
      bool accept(double cost_diff) override {
        return cost_diff <= 0 ||
               Walker<Model>::random_number_generator_->uniform() <
               exp(-cost_diff * beta_);
      }

      /// Set the sampling temperature (also updates beta_).
      void set_temperature(double temperature);
    };

where the selection of a random next state and conditional application
of the proposed transition is implemented in the [`Walker`](../api/markov/walker.yml)
base class.

*NOTE*: For optimization problems it is customary to set the Boltzmann
constant to $`k_B\equiv 1`$ and work with the inverse temperature
$`\beta=1/T`$.

Usage
-----
The `Metropolis` class is used by solvers which rely on Boltzmann
sampling, such as [`ParallelTempering`](../api/solver/parallel-tempering.yml),
[`SimulatedAnnealing`](../api/solver/simulated-annealing.yml) and
[`PopulationAnnealing`](../api/solver/population-annealing.yml).

**Instantiation**: The class of the [model](../api/model/index.md) to be sampled is specified
as a template argument to the `Metropolis` class. This defines both the model
used for cost function evaluation and the type of state (`Model::State_T`) in
the markov chain.

**Preparation**: `Metropolis` instance initialization requires the following
steps prior to calls to the `make_step()` method (inherited from `Walker`):

  * `set_model(&your_model)` must be called with a pointer to an instance of
    the model to use (not owned).
  * `set_random_number_generator(&your_rng)` must be called with a pointer to
    the random number generator to use (now owned).
  * `set_temperature(double)` must be called with the desired sampling
    temperature (the default value is $`T=1`$).
  * `init()` must be invoked after the model and rng have been assigned
    (and before `make_step()` is called).

Both `your_model` and `your_rng` are "not owned", meaning they must be
instantiated by the caller and must outlive the last call to `make_step()`

*NOTE*: This delegation of ownership for the `Model` and
`RandomNumberGenerator` allows the caller to manage the number of instances
for each (e.g., avoid multiple models; rng's as needed for multi-threading).

**Introspection**: As a [`Walker`](../api/markov/walker.yml), each metropolis instance keeps track of the
last state in the markov chain and caches its current energy. In particular,
the calls to `state()` and `cost()` are $`O(1)`$.

Example
-------

    #!cpp
    #include "random/generator.h"
    #include "markov/metropolis.h
    #include "solver/test/test_model.h"

    int main() {
      // Prepare objects not owned by metropolis.
      TestModel model;
      common::RandomNumberGenerator rng;
      rng.seed(42);

      // Initialize the metropolis instance.
      Metropolis<TestModel> metropolis;
      metropolis.set_model(&model);
      metropolis.set_rng(&rng)
      metropolis.set_temperature(0.1);
      metropolis.init();

      // Attempt 50 metropolis steps
      for (int i = 0; i < 50; i++) {
        metropolis.make_step();
        std::cout << metropolis.state() << " " << metropolis.cost() << std::endl;
      }

      return 0;
    }
