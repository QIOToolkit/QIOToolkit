// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <sstream>
#include <vector>

#include "utils/exception.h"
#include "utils/utils.h"
#include "model/graph_model.h"
#include "model/pubo.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
template <class T>
class PuboGrouped
    : public FacedGraphModel<BinaryWithCounter<T>, size_t, CostCache>
{
 public:
  using Base_T = FacedGraphModel<BinaryWithCounter<T>, size_t, CostCache>;
  using State_T = BinaryWithCounter<T>;
  using Transition_T = size_t;
  using Cost_T = CostCache;
  using Graph = Base_T;

  std::string get_identifier() const override { return "pubo_grouped"; }
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

  PuboGrouped() { this->graph_.set_allow_dup_merge(true); }

  void configure(const utils::Json& json) override { Graph::configure(json); }

  void configure(typename Base_T::Configuration_T& configuration)
  {
    Base_T::configure(configuration);
  }

  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    return get_random_binary_state<State_T, PuboGrouped>(rng, *this);
  }

  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    return static_cast<Transition_T>(
        floor(rng.uniform() * static_cast<double>(Graph::node_count())));
  }

  utils::Structure render_state(const State_T& state) const override
  {
    return state.render(this->graph_.output_map());
  }

  Cost_T calculate_cost(const State_T& state) const
  {
    Cost_T cost(0.0);
    for (size_t face_id = 0; face_id < faces().size(); face_id++)
    {
      const auto& f = face(face_id);
      double face_cost = 0.0;
      if (state.zeros.empty())
      {
        // Inspect the spins of nodes in each edge to determine if edge
        // contributes to cost
        for (size_t j = 0; j < f.edge_ids().size(); j++)
        {
          const auto& e = edge(f.edge_ids()[j]);
          bool contains_zero = false;
          for (size_t k = 0; k < e.node_ids().size(); k++)
          {
            if (state.spins[e.node_ids()[k]])
            {
              contains_zero = true;
              break;
            }
          }
          if (!contains_zero)
          {
            face_cost += e.cost();
          }
        }
      }
      else
      {
        // Utilize zeros vector, which counts the number of nodes with spin 0 in
        // each edge
        for (size_t j = 0; j < f.edge_ids().size(); j++)
        {
          int edge_id = f.edge_ids()[j];
          if (state.zeros[edge_id] == 0)
          {
            face_cost += edge(edge_id).cost();
          }
        }
      }
      // Execute type-specific calculations
      switch (f.type())
      {
        case graph::FaceType::Combination:
          cost.value += f.cost() * face_cost;
          // Regular faces do not need cached value
          cost.cache.push_back(0.0);
          break;
        case graph::FaceType::SquaredLinearCombination:
          // Evaluation of the linear combination inside the SLC term is
          // currently stored as face_cost So face_cost must be squared and then
          // scaled by the SLC coefficient f.cost
          cost.value += f.cost() * face_cost * face_cost;
          // SLC faces store 2*S(x)
          cost.cache.push_back(2 * face_cost);
          break;
      }
    }
    return cost;
  }

  Cost_T calculate_cost_difference(const State_T& state,
                                   const Transition_T& transition,
                                   const Cost_T* cost) const
  {
    double diff = 0.0;
    double face_diff = 0.0;
    const auto& n = node(transition);
    const int x = state.spins[transition] ? 0 : 1;
    const int sign = 1 - 2 * x;

    std::function<void(const std::vector<size_t>&, const State_T&)>
        linear_differencer;
    if (x == 0)
    {
      // Transition node currently at 0; would flip to 1
      linear_differencer = [&](const std::vector<size_t>& edge_ids,
                               const State_T& state) {
        for (size_t j = 0; j < edge_ids.size(); j++)
        {
          int edge_id = edge_ids[j];
          if (state.zeros[edge_id] == 1)
          {
            // Edges incident to the transition node with only one zero will be
            // activated
            face_diff += edge(edge_id).cost();
          }
        }
      };
    }
    else
    {
      // Transition node currently at 1; would flip to 0
      linear_differencer = [&](const std::vector<size_t>& edge_ids,
                               const State_T& state) {
        for (size_t j = 0; j < edge_ids.size(); j++)
        {
          int edge_id = edge_ids[j];
          if (state.zeros[edge_id] == 0)
          {
            // Edges incident to the transition node with no zeros will be
            // deactivated
            face_diff -= edge(edge_id).cost();
          }
        }
      };
    }

    for (size_t i = 0; i < node(transition).face_ids().size(); i++)
    {
      int face_id = node(transition).face_ids()[i];
      const auto& f = face(face_id);
      const std::vector<size_t>& edge_ids = n.edge_ids(face_id);
      switch (f.type())
      {
        case graph::FaceType::Combination:
          face_diff = 0;
          linear_differencer(edge_ids, state);
          diff += f.cost() * face_diff;
          break;
        case graph::FaceType::SquaredLinearCombination:
          // Implement delta_r q(x) = Cw_r(1-2x_r)(2S(x) + w_r(1 - 2x_r))
          // Assume like terms have been combined, so edge_ids should contain
          // exactly one element
          const double w_sign = edge(edge_ids[0]).cost() * sign;
          diff += f.cost() * w_sign * (cost->cache[face_id] + w_sign);
          break;
      }
    }
    return Cost_T(diff);
  }

  void apply_transition(const Transition_T& transition, State_T& state,
                        Cost_T* cost) const
  {
    const auto& n = node(transition);
    const int x = state.spins[transition] ? 0 : 1;
    const int sign = 1 - 2 * x;
    if (cost)
    {
      // Update cached sum evaluation for each face containing transition node
      // Does not change the cost value
      for (size_t i = 0; i < n.face_ids().size(); i++)
      {
        int face_id = node(transition).face_ids()[i];
        const auto& f = face(face_id);
        switch (f.type())
        {
          case graph::FaceType::Combination:
            break;
          case graph::FaceType::SquaredLinearCombination:
            const std::vector<size_t>& edge_ids = n.edge_ids(face_id);
            // Implement delta_r 2S(x) = 2w_r(1-2x_r)
            if (!edge_ids.empty())
            {
              // Assume like terms have been combined, so edge_ids should
              // contain exactly one element
              cost->cache[face_id] += 2 * edge(edge_ids[0]).cost() * sign;
            }
            break;
        }
      }
    }
    // Update spin and zero counters
    // If spin is currently 0, then flipping decrements the zero counter
    // Conversely if spin is 1, flipping increments the zero counter
    for (size_t i = 0; i < n.face_ids().size(); i++)
    {
      int face_id = n.face_ids()[i];
      for (size_t j = 0; j < n.edge_ids(face_id).size(); j++)
      {
        state.zeros[n.edge_ids(face_id)[j]] -= sign;
      }
    }
    state.spins[transition] = !state.spins[transition];
  }

  void apply_transition(const Transition_T& transition,
                        State_T& state) const override
  {
    apply_transition(transition, state, nullptr);
  }

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(nodes().size(),
                                    edges().size()) +  // spins and zeros
           utils::vector_values_memory_estimate<double>(
               faces().size());  // cached sums
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(nodes().size());
  }

  virtual void insert_val_to_bbstrees(
      double val, std::multiset<double>& tree_A,
      std::multiset<double>* tree_B) const override
  {
    if (val > 0)
    {
      tree_A.insert(val);
    }
    else if (val < 0)
    {
      tree_B->insert(-val);
    }
  }

  virtual void amend_collation(std::map<int, double>& coeffs,
                               int node_id) const override
  {
    // In binary problems, terms node_id**2 are linear
    coeffs[nodes().size()] += coeffs[node_id];
    coeffs[node_id] = 0;
  }

  double estimate_min_cost_diff() const
  {
    double min_diff = std::numeric_limits<double>::max();
#ifdef _MSC_VER
    // reduction min or max is not implemented yet in all compilers
    #pragma omp parallel
    {
      double min_diff_local = std::numeric_limits<double>::max();
      #pragma omp for
#else
    #pragma omp parallel for reduction(min : min_diff)
#endif
      for (size_t node_id = 0; node_id < nodes().size(); node_id++)
      {
        // Assemble balanced binary search trees of term coefficients
        // in expanded form split according to sign
        std::multiset<double> positive_costs;
        std::multiset<double> negative_costs;
        this->populate_bbstrees(node_id, positive_costs, &negative_costs);
        double local_min =
            utils::least_diff_method(positive_costs, negative_costs);
        if (local_min == 0)
        {
          continue;
        }
#ifdef _MSC_VER
        min_diff_local =
            local_min < min_diff_local ? local_min : min_diff_local;
      }
      #pragma omp critical
      min_diff = min_diff_local < min_diff ? min_diff_local : min_diff;
    }
#else
      min_diff = local_min < min_diff ? local_min : min_diff;
    }
#endif
    if (min_diff == std::numeric_limits<double>::max())
    {
      // If min_diff was not changed, all cost differences must be 0.
      min_diff = (MIN_DELTA_DEFAULT);
    }
    return min_diff;
  }

 protected:
  using Graph::edge;
  using Graph::edges;
  using Graph::face;
  using Graph::faces;
  using Graph::node;
  using Graph::nodes;
};

}  // namespace model
