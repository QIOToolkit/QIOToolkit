
#pragma once

#include <cmath>
#include <vector>
#include <memory>

#include "markov/cluster_walker.h"
#include "utils/utils.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// Quantum Algorithm
///

template <class Model>
class QuantumWalker : public ClusterWalker<Model>
{
 public:
  using State = typename Model::Model::State_T;
  using Transition = typename Model::Model::Transition_T;

  /// Create a Metropolis instance with uninitialized model and state.
  QuantumWalker() : beta_(10), bond_prob_(0.9) {}

  double beta() const { return beta_; }

  void set_beta(double beta) { beta_ = beta; }

  double bond_probability() const { return bond_prob_; }

  void set_bond_probability(double bond_prob) { bond_prob_ = bond_prob; }

  /// Decide whether to accept a given cost increase.
  bool accept(const typename Model::Cost_T& cost_diff,
              ::utils::RandomGenerator& rng) override
  {
    const double FLOATING_POINT_CALCULATION_ERROR = 10E-12;

    if (cost_diff < FLOATING_POINT_CALCULATION_ERROR ||
        rng.uniform() < std::exp(-beta_ * cost_diff))
      return true;
    else
      return false;
  }

  /// Decide whether to accept addition to cluster.
  bool accept_cluster(::utils::RandomGenerator& rng) override 
  { 
      return rng.uniform() < bond_prob_; 
  }

  /// Estimate memory consumtion using model parameters
  static size_t memory_estimate(const Model& model)
  {
    return sizeof(QuantumWalker<Model>) - sizeof(ClusterWalker<Model>) +
           ClusterWalker<Model>::memory_estimate(model);
  }


 private:
  double beta_;       // beta for accepting cluster flip
  double bond_prob_;  // the probabilty to form bonds b/w aligned spins
};

}  // namespace markov
