
#pragma once

#include <random>
#include <string>
#include <vector>

#include "utils/operating_system.h"
#include "markov/state.h"
#include "markov/transition.h"
#include "model/graph_model.h"

namespace model
{
class BlumeCapel;

class BlumeCapelState : public ::markov::State
{
 public:
  BlumeCapelState() {}
  BlumeCapelState(size_t N, size_t M) : spins(N), zeros(M), signs(M) {}

  int term(size_t edge_id) const;

  utils::Structure render() const override;

  void copy_state_only(const BlumeCapelState& other) { spins = other.spins; }

  static size_t memory_estimate(size_t N, size_t M)
  {
    return sizeof(BlumeCapelState) +
           utils::vector_values_memory_estimate<int>(N) +
           utils::vector_values_memory_estimate<size_t>(M) +
           utils::vector_values_memory_estimate<bool>(
               M);  //  signs memory estimation
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N, 0);
  }

  const std::vector<int>& get_spins() const { return spins; }

 private:
  friend class BlumeCapel;
  std::vector<int> spins;
  std::vector<size_t> zeros;
  std::vector<bool> signs;
};

class BlumeCapelTransition : public ::markov::Transition
{
 public:
  BlumeCapelTransition() : spin_id_(0), value_(0) {}
  BlumeCapelTransition(size_t spin_id, int value)
      : spin_id_(spin_id), value_(value)
  {
  }

  size_t spin_id() const { return spin_id_; }
  int value() const { return value_; }

  bool operator==(const BlumeCapelTransition& trans) const
  {
    return spin_id_ == trans.spin_id_ && value_ == trans.value_;
  }

 private:
  size_t spin_id_;
  int value_;
};

inline bool same_state_value(const BlumeCapelState& state1,
                             const BlumeCapelState& state2,
                      const BlumeCapelTransition& transition)
{
  return state1.get_spins()[transition.spin_id()] ==
         state2.get_spins()[transition.spin_id()];
}

class BlumeCapel : public GraphModel<BlumeCapelState, BlumeCapelTransition>
{
 public:
  using State_T = BlumeCapelState;
  using Transition_T = BlumeCapelTransition;
  using Graph = GraphModel<State_T, Transition_T>;

  std::string get_identifier() const override { return "blume-capel"; }
  std::string get_version() const override { return "1.0"; }

  double cost_term(const State_T& state, size_t j) const;

  double cost_term(const State_T& state, size_t j, size_t modified_spin,
                   int modified_value) const;

  double calculate_cost(const State_T& state) const override;

  double calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override;

  State_T get_random_state(utils::RandomGenerator& rng) const override;

  Transition_T get_random_transition(const State_T& state,
                                     utils::RandomGenerator& rng) const override;

  void apply_transition(const Transition_T& transition,
                        State_T& state) const override;

  void configure(const utils::Json& json) override;

  void configure(Graph::Configuration_T& configuration)
  {
    Graph::configure(configuration);
  }
  
  utils::Structure render_state(const State_T& state) const override;

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(nodes().size(), edges().size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(nodes().size());
  }
};
REGISTER_MODEL(BlumeCapel);

}  // namespace model

template <>
struct std::hash<model::BlumeCapelTransition>
{
  std::size_t operator()(
      const model::BlumeCapelTransition& trans) const noexcept
  {
    return utils::get_combined_hash(trans.spin_id(), trans.value());
  }
};
