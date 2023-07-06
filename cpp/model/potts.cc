
#include "model/potts.h"

#include <cassert>

namespace model
{
void PottsTransition::apply(PottsState& state) const
{
  assert(spin_id_ < state.spins.size());
  state.spins[spin_id_] = value_;
}

double Potts::calculate_term(const PottsState& state, size_t edge_id) const
{
  const auto& e = edge(edge_id);
  if (e.node_ids().size() > 1)
  {
    size_t first_spin_id = e.node_ids()[0];
    size_t value = state.spins[first_spin_id];
    for (size_t spin_id : e.node_ids())
    {
      if (state.spins[spin_id] != value)
      {
        return 0;
      }
    }
  }
  return e.cost();
}

double Potts::calculate_term(const PottsState& state, size_t edge_id,
                             const PottsTransition& transition) const
{
  const auto& e = edge(edge_id);
  if (e.node_ids().size() > 1)
  {
    for (size_t spin_id : e.node_ids())
    {
      if (spin_id == transition.spin_id()) continue;
      if (state.spins[spin_id] != transition.value())
      {
        return 0;
      }
    }
  }
  return e.cost();
}

double Potts::calculate_cost(const PottsState& state) const
{
  double cost = 0.0;
  for (size_t edge_id = 0; edge_id < edges().size(); edge_id++)
  {
    cost += calculate_term(state, edge_id);
  }
  return cost;
}

double Potts::calculate_cost_difference(const PottsState& state,
                                        const PottsTransition& transition) const
{
  double diff = 0.0;
  for (size_t edge_id : node(transition.spin_id()).edge_ids())
  {
    diff -= calculate_term(state, edge_id);
    diff += calculate_term(state, edge_id, transition);
  }
  return diff;
}

PottsState Potts::get_random_state(utils::RandomGenerator& rng) const
{
  PottsState state(nodes().size());
  for (size_t i = 0; i < nodes().size(); i++)
  {
    state.spins[i] = (size_t)floor(rng.uniform() * q_);
  }
  return state;
}

PottsTransition Potts::get_random_transition(const PottsState& state,
                                             utils::RandomGenerator& rng) const
{
  size_t spin_id = static_cast<size_t>(
      floor(rng.uniform() * static_cast<double>(state.spins.size())));
  size_t value =
      (state.spins[spin_id] + 1 + int(rng.uniform() * (q_ - 1))) % q_;
  return PottsTransition(spin_id, value);
}

void Potts::apply_transition(const PottsTransition& transition,
                             PottsState& state) const
{
  transition.apply(state);
}

void Potts::configure(const utils::Json& json)
{
  GraphModel<PottsState, PottsTransition>::configure(json);
  this->param(json, "q", q_).description("number of potts states").required();
}

}  // namespace model
