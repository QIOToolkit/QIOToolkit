// Copyright (c) Microsoft. All Rights Reserved.
#include "model/ising.h"

namespace model
{
utils::Structure IsingState::render() const
{
  utils::Structure s(utils::Structure::OBJECT);
  for (size_t i = 0; i < spins.size(); i++)
  {
    s[std::to_string(i)] = spins[i] ? -1 : 1;
  }
  return s;
}

utils::Structure IsingState::render(const std::map<int, int>& ids_map) const
{
  utils::Structure s(utils::Structure::OBJECT);
  for (size_t i = 0; i < spins.size(); i++)
  {
    s[std::to_string(ids_map.at((int)i))] = spins[i] ? -1 : 1;
  }
  return s;
}

void Ising::apply_transition(const size_t& spin_id, IsingState& state) const
{
  state.spins[spin_id] = !state.spins[spin_id];
}

double Ising::get_term(const IsingState& state, size_t term_id) const
{
  return get_ising_term<State_T, Ising>(state, term_id, *this);
}

void IsingTermCached::apply_transition(const size_t& spin_id,
                                       IsingTermCachedState& state) const
{
  state.spins[spin_id] = !state.spins[spin_id];
  for (size_t j : node(spin_id).edge_ids())
  {
    state.terms[j] = !state.terms[j];
  }
}

double IsingTermCached::get_term(const IsingTermCachedState& state,
                                 size_t term_id) const
{
  if (state.terms.empty())
  {
    return get_ising_term<State_T, IsingTermCached>(state, term_id, *this);
  }
  double c = edge(term_id).cost();
  return state.terms[term_id] ? -c : c;
}
}  // namespace model
