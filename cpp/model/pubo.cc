
#include "model/pubo.h"

namespace model
{
utils::Structure Binary::render() const
{
  utils::Structure s(utils::Structure::OBJECT);
  for (size_t i = 0; i < spins.size(); i++)
  {
    s[std::to_string(i)] = spins[i] ? 0 : 1;
  }
  return s;
}

utils::Structure Binary::render(const std::map<int, int>& ids_map) const
{
  utils::Structure s(utils::Structure::OBJECT);

  for (size_t i = 0; i < spins.size(); i++)
  {
    s[std::to_string(ids_map.at((int)i))] = spins[i] ? 0 : 1;
  }
  return s;
}

double Pubo::calculate_cost(const State_T& state) const
{
  double cost = 0.0;
  for (size_t j = 0; j < Graph::edges().size(); j++)
  {
    const auto& e = Graph::edge(j);
    bool zero = false;
    for (size_t i : e.node_ids())
    {
      if (state.spins[i])
      {
        zero = true;
        break;
      }
    }
    if (!zero)
    {
      cost += e.cost();
    }
  }
  return cost;
}

double Pubo::calculate_cost_difference(const State_T& state,
                                       const Transition_T& transition) const
{
  double diff = 0.0;
  const auto& node = Graph::node(transition);
  for (size_t j : node.edge_ids())
  {
    const auto& e = Graph::edge(j);
    bool zero = false;
    for (size_t i : e.node_ids())
    {
      if (i != transition && state.spins[i])
      {
        zero = true;
        break;
      }
    }
    if (!zero)
    {
      diff += state.spins[transition] ? e.cost() : -e.cost();
    }
  }
  return diff;
}

void Pubo::apply_transition(const Transition_T& transition,
                            State_T& state) const
{
  state.spins[transition] = !state.spins[transition];
}
}  // namespace model
