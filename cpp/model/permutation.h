
#pragma once

#include <algorithm>
#include <sstream>
#include <vector>

#include "utils/operating_system.h"
#include "utils/random_generator.h"
#include "markov/state.h"
#include "markov/transition.h"

namespace model
{
class Permutation : public ::markov::State
{
 public:
  Permutation() {}

  Permutation(size_t N);

  static Permutation random(size_t N, utils::RandomGenerator& rng);

  size_t operator[](size_t i) const;

  size_t size() const;

  // Take the node currently at position a and move it after node b.
  void move_node(size_t a, size_t b);

  // Swap the nodes at position a and b.
  void swap_nodes(size_t a, size_t b);

  // Swap the target of a with the source of b (this un-/crosses the
  // original edges and reverses the arc a..b.
  void swap_edges(size_t a, size_t b);

  utils::Structure render() const override;

  void copy_state_only(const Permutation& other)
  {
    node_ids_ = other.node_ids_;
  }

  static size_t memory_estimate(size_t N)
  {
    return sizeof(Permutation) + utils::vector_values_memory_estimate<size_t>(N);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N);
  }

 private:
  std::vector<size_t> node_ids_;
};

}  // namespace model
