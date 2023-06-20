// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>
#include <vector>

#include "utils/operating_system.h"
#include "markov/state.h"
#include "markov/transition.h"
#include "model/graph_model.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// Potts state representation
///
/// Represents N potts spins as a vector<int>.
class PottsState : public ::markov::State
{
 public:
  PottsState() {}

  /// Initialize a state of N potts spins to 0.
  PottsState(size_t N) : spins(N) {}

  utils::Structure render() const override
  {
    utils::Structure s;
    s = spins;
    return s;
  }

  void copy_state_only(const PottsState& other) { spins = other.spins; }

  static size_t memory_estimate(size_t N)
  {
    return sizeof(PottsState) + utils::vector_values_memory_estimate<size_t>(N);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N);
  }

  /// Potts spin container.
  std::vector<size_t> spins;
};

////////////////////////////////////////////////////////////////////////////////
/// Potts transition representation
///
/// Which spin to change and to what value.
class PottsTransition : public ::markov::Transition
{
 public:
  PottsTransition() {}

  /// Initialize a transition of `spin_id` to `value`.
  ///
  /// NOTE: The choice of `value` (both its range and w.r.t. the current value
  /// of `spins[spin_id]` is handled by the model.
  PottsTransition(size_t spin_id, size_t value)
      : spin_id_(spin_id), value_(value)
  {
  }

  /// Apply the transition to the given state.
  void apply(PottsState& state) const;

  size_t spin_id() const { return spin_id_; }
  size_t value() const { return value_; }

 private:
  /// Which spin to change.
  size_t spin_id_;

  /// What value to change it to.
  size_t value_;
};

////////////////////////////////////////////////////////////////////////////////
/// Potts Model
///
/// This model implements a hamiltonian of the form
///
/// \$ \mathcal{H} = \sum_j c_j \delta_{s_{j,1}, s{j,2}, \ldots} \$,
///
/// where \$\delta\$ is the kronecker delta, which is 1 iff all its subscripts
/// are identical (and 0 otherwise). That is: Each term in this cost function
/// will contribute only if all Potts spins participating in it are of the same
/// value.
///
class Potts : public GraphModel<PottsState, PottsTransition>
{
 public:
  std::string get_identifier() const override { return "potts"; }
  std::string get_version() const override { return "1.0"; }

  Potts() : q_(10) {}

  /// Calculate the Potts Hamiltonian.
  double calculate_cost(const PottsState& state) const override;

  /// Recompute the parts affected by `transition`.
  double calculate_cost_difference(
      const PottsState& state,
      const PottsTransition& transition) const override;

  /// Return a random Potts state.
  ///
  /// This function decides on the number of potts spins and the possible
  /// values of each to return (according to the model configuration).
  PottsState get_random_state(utils::RandomGenerator& rng) const override;

  /// Create a random transition for `state`.
  PottsTransition get_random_transition(
      const PottsState& state, utils::RandomGenerator& rng) const override;

  /// Change `state` according to `transition`.
  void apply_transition(const PottsTransition& transition,
                        PottsState& state) const override;

  /// Serialize `q` and the underlying graph.
  void configure(const utils::Json& json) override;

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(nodes().size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::memory_estimate(nodes().size());
  }

 private:
  double calculate_term(const PottsState& state, size_t edge_id) const;
  double calculate_term(const PottsState& state, size_t edge_id,
                        const PottsTransition& transition) const;

  int q_;
};
REGISTER_MODEL(Potts);

}  // namespace model
