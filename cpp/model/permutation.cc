
#include "model/permutation.h"

namespace model
{
Permutation::Permutation(size_t N) : node_ids_(N)
{
  size_t i = 0;
  for (auto& node_id : node_ids_)
  {
    node_id = i++;
  }
}

Permutation Permutation::random(size_t N, utils::RandomGenerator& rng)
{
  Permutation p(N);
  std::shuffle(p.node_ids_.begin(), p.node_ids_.end(), rng);
  return p;
}

size_t Permutation::operator[](size_t i) const { return node_ids_[i]; }

size_t Permutation::size() const { return node_ids_.size(); }

void Permutation::move_node(size_t a, size_t b)
{
  size_t moved_value = node_ids_[a];
  while (a != b)
  {
    if (a < node_ids_.size() - 1)
    {
      node_ids_[a] = node_ids_[a + 1];
      a++;
    }
    else
    {
      node_ids_[a] = node_ids_[0];
      a = 0;
    }
  }
  node_ids_[b] = moved_value;
}

void Permutation::swap_nodes(size_t a, size_t b)
{
  std::swap(node_ids_[a], node_ids_[b]);
}

void Permutation::swap_edges(size_t a, size_t b)
{
  while (a != b)
  {
    std::swap(node_ids_[a], node_ids_[b]);
    a++;
    if (a == node_ids_.size()) a = 0;
    if (a == b) break;
    b = (b == 0) ? node_ids_.size() - 1 : b - 1;
  }
}

utils::Structure Permutation::render() const
{
  utils::Structure output;
  output = node_ids_;
  return output;
}

}  // namespace model
