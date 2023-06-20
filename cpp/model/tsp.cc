// Copyright (c) Microsoft. All Rights Reserved.
#include "model/tsp.h"

namespace model
{
TspTransition TspTransition::random(size_t N, utils::RandomGenerator& rng)
{
  Type type = Type(floor(rng.uniform() * 3));
  size_t a = static_cast<size_t>(rng.uniform() * static_cast<double>(N));
  size_t b = static_cast<size_t>(rng.uniform() * static_cast<double>(N));
  return TspTransition(type, a, b);
}

void TspTransition::apply(Permutation& permutation) const
{
  switch (type_)
  {
    case MOVE_NODE:
      permutation.move_node(a_, b_);
      break;
    case SWAP_NODES:
      permutation.swap_nodes(a_, b_);
      break;
    case SWAP_EDGES:
      permutation.swap_edges(a_, b_);
      break;
  }
}

std::string TspTransition::get_class_name() const { return "TspTransition"; }

double Tsp::calculate_cost(const Permutation& p) const
{
  double cost = dist_[p[p.size() - 1]][p[0]];
  for (size_t i = 0; i < p.size() - 1; i++)
  {
    cost += dist_[p[i]][p[i + 1]];
  }
  return cost;
}

double Tsp::calculate_cost_difference(const Permutation& p,
                                      const TspTransition& t) const
{
  double diff = 0.0;
  size_t N = p.size();
  size_t a = p[t.a()];
  size_t before_a = p[(t.a() + N - 1) % N];
  size_t after_a = p[(t.a() + 1) % N];
  size_t b = p[t.b()];
  size_t before_b = p[(t.b() + N - 1) % N];
  size_t after_b = p[(t.b() + 1) % N];

  switch (t.type())
  {
    case TspTransition::MOVE_NODE:
      if (t.a() == t.b() || (t.a() + N - 1) % N == t.b())
      {
        break;
      }
      diff -= dist_[before_a][a];
      diff -= dist_[a][after_a];
      diff += dist_[before_a][after_a];
      diff -= dist_[b][after_b];
      diff += dist_[b][a];
      diff += dist_[a][after_b];
      break;
    case TspTransition::SWAP_NODES:
      if (t.a() == t.b()) break;
      if ((t.b() + 1) % N != t.a())
      {
        diff -= dist_[before_a][a];
        diff += dist_[before_a][b];
        diff -= dist_[b][after_b];
        diff += dist_[a][after_b];
      }

      if ((t.a() + 1) % N != t.b())
      {
        diff -= dist_[a][after_a];
        diff += dist_[b][after_a];
        diff -= dist_[before_b][b];
        diff += dist_[before_b][a];
      }
      break;
    case TspTransition::SWAP_EDGES:
      if ((t.b() + 1) % N == t.a())
      {
        // Complete reverse; nothing changes.
        break;
      }
      // This assumes the graph is undirected, otherwise we would need to
      // account for the cost of reversing [a..b]
      diff -= dist_[before_a][a];
      diff += dist_[before_a][b];
      diff -= dist_[b][after_b];
      diff += dist_[a][after_b];
      break;
  }

  Permutation p2 = p;
  t.apply(p2);

  return diff;
}

Permutation Tsp::get_random_state(utils::RandomGenerator& rng) const
{
  return Permutation::random(dist_.size(), rng);
}

TspTransition Tsp::get_random_transition(const Permutation& p,
                                         utils::RandomGenerator& rng) const
{
  return TspTransition::random(p.size(), rng);
}

void Tsp::apply_transition(const TspTransition& transition,
                           Permutation& p) const
{
  transition.apply(p);
}

void Tsp::configure(const utils::Json& json)
{
  markov::Model<Permutation, TspTransition>::configure(json);
  this->param(json, "dist", dist_)
      .description("adjacency matrix of distances")
      .required();
}

void Tsp::configure(Configuration_T& config)
{
  markov::Model<Permutation, TspTransition>::configure(config);
  dist_ = std::move(Configuration_T::Get_Dist::get(config));
}

}  // namespace model
