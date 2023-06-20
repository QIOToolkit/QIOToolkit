// Copyright (c) Microsoft. All Rights Reserved.
#include "model/ising_grouped.h"

namespace model
{
using Cost_T = CostCache;

Cost_T IsingGrouped::calculate_cost(const State_T& state) const
{
  Cost_T cost(0.0);
  for (size_t face_id = 0; face_id < faces().size(); face_id++)
  {
    const auto& f = face(face_id);
    double face_cost = 0.0;
    for (size_t j = 0; j < f.edge_ids().size(); j++)
    {
      face_cost += get_term(state, f.edge_ids()[j]);
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
        // Evaluation of the linear combination inside the SLC term is currently
        // stored as face_cost So face_cost must be squared and then scaled by
        // the SLC coefficient f.cost
        cost.value += f.cost() * face_cost * face_cost;
        // SLC faces store 2*S(x)
        cost.cache.push_back(2 * face_cost);
        break;
    }
  }
  return cost;
}

Cost_T IsingGrouped::calculate_cost_difference(const State_T& state,
                                               const Transition_T& transition,
                                               const Cost_T* cost) const
{
  double diff = 0.0;
  double face_diff = 0.0;
  const auto& n = node(transition);
  const int x = state.spins[transition] ? -1 : 1;
  const int sign = -2 * x;

  for (size_t i = 0; i < node(transition).face_ids().size(); i++)
  {
    int face_id = node(transition).face_ids()[i];
    const auto& f = face(face_id);
    const std::vector<size_t>& edge_ids = n.edge_ids(face_id);
    switch (f.type())
    {
      case graph::FaceType::Combination:
        face_diff = 0.0;
        for (size_t j = 0; j < edge_ids.size(); j++)
        {
          face_diff += get_term(state, edge_ids[j]);
        }
        diff += 2.0 * f.cost() * (-face_diff);
        break;
      case graph::FaceType::SquaredLinearCombination:
        // Implement delta_r q(x) = C(-2w_rx_r)(2S(x) + (-2w_rx_r))
        // Assume like terms have been combined, so edge_ids should contain
        // exactly one element
        const double w_sign = edge(edge_ids[0]).cost() * sign;
        diff += f.cost() * w_sign * (cost->cache[face_id] + w_sign);
        break;
    }
  }
  return Cost_T(diff);
}

void IsingGrouped::apply_transition(const Transition_T& transition,
                                    State_T& state, Cost_T* cost) const
{
  const auto& n = node(transition);
  if (cost)
  {
    const int x = state.spins[transition] ? -1 : 1;
    const int sign = -2 * x;
    // Update cached sum evaluation for each face containing transition node
    // Does not change the cost value
    for (size_t i = 0; i < n.face_ids().size(); i++)
    {
      int face_id = n.face_ids()[i];
      const auto& f = face(face_id);
      switch (f.type())
      {
        case graph::FaceType::Combination:
          break;
        case graph::FaceType::SquaredLinearCombination:
          const std::vector<size_t>& edge_ids = n.edge_ids(face_id);
          // Implement delta_r 2S(x) = 2(-2w_rx_r)
          if (!edge_ids.empty())
          {
            // Assume like terms have been combined, so edge_ids should contain
            // exactly one element
            cost->cache[face_id] += 2 * edge(edge_ids[0]).cost() * sign;
          }
          break;
      }
    }
  }
  // Update spin and terms cache
  state.spins[transition] = !state.spins[transition];
  for (size_t i = 0; i < n.face_ids().size(); i++)
  {
    for (size_t j = 0; j < n.edge_ids(n.face_ids()[i]).size(); j++)
    {
      int edge_id = n.edge_ids(n.face_ids()[i])[j];
      state.terms[edge_id] = !state.terms[edge_id];
    }
  }
}

double IsingGrouped::get_term(const State_T& state, size_t edge_id) const
{
  if (state.terms.empty())
  {
    return get_ising_term<State_T, IsingGrouped>(state, edge_id, *this);
  }
  double c = edge(edge_id).cost();
  return state.terms[edge_id] ? -c : c;
}

void IsingGrouped::insert_val_to_bbstrees(double val,
                                          std::multiset<double>& tree_A,
                                          std::multiset<double>*) const
{
  tree_A.insert(std::abs(val));
}

void IsingGrouped::amend_collation(std::map<int, double>& coeffs,
                                   int node_id) const
{
  // In spin problems, terms node_id**2 are constant
  // and do not contribute to cost differences
  coeffs[node_id] = 0;
}

double IsingGrouped::estimate_min_cost_diff() const
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
    for (size_t node_id = 0; node_id < node_count(); node_id++)
    {
      std::multiset<double> sorted_costs;
      this->populate_bbstrees(node_id, sorted_costs);
      double diff_local = utils::least_diff_method(sorted_costs);
      sorted_costs.clear();
      if (diff_local == 0)
      {
        continue;
      }
#ifdef _MSC_VER
      min_diff_local =
          diff_local < min_diff_local ? diff_local : min_diff_local;
    }
    #pragma omp critical
    min_diff = min_diff_local < min_diff ? min_diff_local : min_diff;
  }
#else
    min_diff = diff_local < min_diff ? diff_local : min_diff;
  }
#endif
  if (min_diff == std::numeric_limits<double>::max())
  {
    min_diff = (MIN_DELTA_DEFAULT);
  }
  return min_diff * 2;
}

}  // namespace model