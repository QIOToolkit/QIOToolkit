
#pragma once

#include "utils/config.h"
#include "markov/state.h"
#include "markov/transition.h"
#include "model/graph_model.h"

namespace examples
{
class SoftSpinState : public ::markov::State
{
 public:
  std::vector<double> spin;
  utils::Structure render() const override { return spin; }
  utils::Structure get_status() const override { return spin; }
  std::string get_class_name() const override { return "SoftSpinState"; }
  static size_t memory_estimate(size_t N)
  {
    return sizeof(SoftSpinState) +
           utils::vector_values_memory_estimate<double>(N);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N);
  }
};

class SoftSpinTransition : public ::markov::Transition
{
 public:
  SoftSpinTransition() : spin_id(0), new_value(0) {}
  int spin_id;
  double new_value;
  bool operator<(const SoftSpinTransition& trans) const
  {
    if (spin_id == trans.spin_id)
    {
      return new_value < trans.new_value;
    }
    else
    {
      return spin_id < trans.spin_id;
    }
  }
};

class SoftSpin : public ::model::GraphModel<SoftSpinState, SoftSpinTransition>
{
 public:
  using State_T = SoftSpinState;
  using Transition_T = SoftSpinTransition;
  using Graph = ::model::GraphModel<State_T, Transition_T>;

  std::string get_identifier() const override { return "softspin"; }
  std::string get_version() const override { return "0.1"; }

  void configure(const utils::Json& json) override { Graph::configure(json); }

  void configure(Configuration_T& configuration)
  {
    Graph::configure(configuration);
  }

  double calculate_cost(const State_T& state) const override
  {
    double cost = 0;
    for (auto e : edges())
    {
      double term = e.cost();
      for (auto spin_id : e.node_ids())
      {
        term *= state.spin[spin_id];
      }
      cost += term;
    }
    return cost;
  }

  double calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override
  {
    double diff = 0;
    for (auto edge_id : node(transition.spin_id).edge_ids())
    {
      const auto& e = edge(edge_id);
      double term_before = e.cost();
      double term_after = e.cost();
      for (auto spin_id : e.node_ids())
      {
        term_before *= state.spin[spin_id];
        if (spin_id == transition.spin_id)
        {
          term_after *= transition.new_value;
        }
        else
        {
          term_after *= state.spin[spin_id];
        }
      }
      diff += term_after - term_before;
    }
    return diff;
  }

  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    State_T state;
    state.spin.resize(nodes().size());
    for (size_t i = 0; i < nodes().size(); i++)
    {
      state.spin[i] = rng.uniform() * 2.0 - 1;
    }
    return state;
  }

  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    Transition_T transition;
    transition.spin_id =
        (size_t)floor(rng.uniform() * static_cast<double>(nodes().size()));
    transition.new_value = rng.uniform() * 2.0 - 1;
    return transition;
  }

  void apply_transition(const Transition_T& transition,
                        State_T& state) const override
  {
    state.spin[transition.spin_id] = transition.new_value;
  }

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(nodes().size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(nodes().size());
  }
};

}  // namespace examples

template <>
struct std::hash<examples::SoftSpinTransition>
{
  std::size_t operator()(
      const examples::SoftSpinTransition& trans) const noexcept
  {
    return utils::get_combined_hash(trans.spin_id, trans.new_value);
  }
};
