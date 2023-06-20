// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <sstream>
#include <vector>

#include "utils/exception.h"
#include "utils/utils.h"
#include "model/graph_model.h"
#include "model/ising.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
class IsingGrouped
    : public FacedGraphModel<IsingTermCachedState, size_t, CostCache>
{
 public:
  using Base_T = FacedGraphModel<IsingTermCachedState, size_t, CostCache>;
  using State_T = IsingTermCachedState;
  using Transition_T = size_t;
  using Cost_T = CostCache;
  using Graph = Base_T;

  std::string get_identifier() const override { return "ising_grouped"; }
  std::string get_version() const override { return "1.1"; }
  void match_version(const std::string& version) override
  {
    if (version.compare("1.0") == 0 || version.compare(get_version()) == 0)
    {
      return;
    }
    throw utils::ValueException(
        "Expecting `version` equals 1.0 or 1.1, but found: " + version);
  }

  void configure(const utils::Json& json) override { Graph::configure(json); }

  void configure(typename Base_T::Configuration_T& configuration)
  {
    Base_T::configure(configuration);
  }

  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    return get_random_spin_state<State_T, IsingGrouped>(rng, *this);
  }

  /// Return a random Single spin update.
  ///
  /// We represent a single spin update as merely the index of the
  /// spin variable that will flip.
  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    return static_cast<Transition_T>(rng.uniform() *
                                     static_cast<double>(Graph::node_count()));
  }

  utils::Structure render_state(const State_T& state) const override
  {
    return state.render(this->graph_.output_map());
  }

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(Graph::node_count(), Graph::edge_count()) +
           utils::vector_values_memory_estimate<double>(
               Graph::faces().size());  // cached sums
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(this->graph_.nodes().size());
  }

  Cost_T calculate_cost(const State_T& state) const;

  Cost_T calculate_cost_difference(const State_T& state,
                                   const Transition_T& transition,
                                   const Cost_T* cost) const override;

  void apply_transition(const Transition_T& transition, State_T& state,
                        Cost_T* cost) const override;

  void apply_transition(const Transition_T& transition,
                        State_T& state) const override
  {
    apply_transition(transition, state, nullptr);
  }

  /// Evaluate the term with id `edge_id`.
  double get_term(const State_T& state, size_t edge_id) const;

  virtual void insert_val_to_bbstrees(
      double val, std::multiset<double>& tree_A,
      std::multiset<double>* tree_B = nullptr) const override;

  virtual void amend_collation(std::map<int, double>& coeffs,
                               int node_id) const override;

  double estimate_min_cost_diff() const;
};

}  // namespace model
