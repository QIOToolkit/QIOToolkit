
#include "model/blume_capel.h"

namespace model
{
int BlumeCapelState::term(size_t edge_id) const
{
  if (zeros[edge_id] > 0) return 0;
  return signs[edge_id] ? -1 : 1;
}

utils::Structure BlumeCapelState::render() const
{
  utils::Structure s;
  s = spins;
  return s;
}

double BlumeCapel::cost_term(const State_T& state, size_t j) const
{
  return state.term(j) * edge(j).cost();
}

double BlumeCapel::cost_term(const State_T& state, size_t j,
                             size_t modified_spin, int modified_value) const
{
  int before = state.spins[modified_spin];
  int after = modified_value;
  size_t zeros = state.zeros[j];

  // We are changing a spin to 0, the term will be null.
  if (after == 0) return 0;

  // The term has zeros and the transition is not removing any
  if (zeros > 0 && before != 0) return 0;

  // The term already has zeros but the transition is not removing all
  size_t count = (size_t)std::count(edge(j).node_ids().begin(),
                                    edge(j).node_ids().end(), modified_spin);
  if (zeros > count) return 0;

  const double& cost = edge(j).cost();

  // The change is toggling the sign (cost : -cost are deliberately reversed
  // here).
  if ((count & 1) && (before == -1) != (after == -1))
  {
    return state.signs[j] ? cost : -cost;
  }

  // The change is not toggling the sign.
  return state.signs[j] ? -cost : cost;
}

double BlumeCapel::calculate_cost(const State_T& state) const
{
  double cost = 0.0;
  for (size_t j = 0; j < edges().size(); j++)
  {
    cost += cost_term(state, j);
  }
  return cost;
}

double BlumeCapel::calculate_cost_difference(
    const State_T& state, const Transition_T& transition) const
{
  size_t modified_spin = transition.spin_id();
  int modified_value = transition.value();
  double difference = 0.0;
  size_t last_j = UINT32_MAX;
  for (size_t j : node(modified_spin).edge_ids())
  {
    if (j == last_j) continue;
    difference += cost_term(state, j, modified_spin, modified_value) -
                  cost_term(state, j);
    last_j = j;
  }
  return difference;
}

BlumeCapel::State_T BlumeCapel::get_random_state(
    utils::RandomGenerator& rng) const
{
  std::uniform_real_distribution<double> uniform(0, 1);
  auto state = State_T(nodes().size(), edges().size());
  for (size_t i = 0; i < nodes().size(); i++)
  {
    int value = (int)floor(rng.uniform() * 3) - 1;
    state.spins[i] = value;
  }
  for (size_t j = 0; j < edges().size(); j++)
  {
    for (size_t i : edge(j).node_ids())
    {
      if (state.spins[i] == 0)
      {
        state.zeros[j]++;
      }
      if (state.spins[i] == -1)
      {
        state.signs[j] = !state.signs[j];
      }
    }
  }
  return state;
}

BlumeCapel::Transition_T BlumeCapel::get_random_transition(
    const State_T& state, utils::RandomGenerator& rng) const
{
  std::uniform_real_distribution<double> uniform(0, 1);
  size_t spin_id =
      static_cast<size_t>(rng.uniform() * static_cast<double>(nodes().size()));
  int value = (state.spins[spin_id] + 1 + int(rng.uniform() * 2)) % 3 - 1;
  return {spin_id, value};
}

void BlumeCapel::apply_transition(const Transition_T& transition,
                                  State_T& state) const
{
  size_t i = transition.spin_id();
  int before = state.spins[i];
  int after = transition.value();
  const auto& edges = node(i).edge_ids();
  for (size_t j : edges)
  {
    if (before == 0 && after != 0)
    {
      state.zeros[j]--;
    }
    if (before != 0 && after == 0)
    {
      state.zeros[j]++;
    }
    auto count =
        std::count(edge(j).node_ids().begin(), edge(j).node_ids().end(), i);
    if ((count & 1) && (before == -1) != (after == -1))
    {
      state.signs[j] = !state.signs[j];
    }
  }
  state.spins[i] = after;
}

void BlumeCapel::configure(const utils::Json& json) { Graph::configure(json); }

utils::Structure BlumeCapel::render_state(const State_T& state) const
{
  std::stringstream status;
  bool first = true;
  for (size_t i = 0; i < state.spins.size(); i++)
  {
    if (state.spins[i] != 0)
    {
      if (first)
        first = false;
      else
        status << ", ";

      status << std::to_string(i) << (state.spins[i] < 0 ? "=-1" : "=+1");
    }
  }
  return status.str();
}

}  // namespace model
