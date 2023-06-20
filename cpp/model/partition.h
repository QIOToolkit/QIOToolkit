// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <algorithm>
#include <sstream>
#include <vector>

#include "utils/random_generator.h"
#include "markov/state.h"
#include "markov/transition.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// Partition state
///
/// This implements a ::markov::State that is a partition of N elements
/// into two sets (with the first one of size K). It is optimized for two
/// operations:
///
/// 1) Querying whether a given number is in the first set
/// 2) Swapping two random elements from different groups
///
/// For this, two internal vectors of the respective elements are kept. These
/// are NOT sorted and the class does not provide an interface to select a
/// specific number to swap (they can only be selected by their index, while
/// the current ordering is unknown).
///
/// NOTE: If you need more sets or don't want the respective set sizes to be
/// fixed, the PottsState is likely a better candidate for your use case.
///
class Partition : public ::markov::State
{
 public:
  /// Create an uninitialized partition
  Partition() {}

  /// Generate a random partition of N elements with the first set of size K
  static Partition random(size_t N, size_t K, utils::RandomGenerator& rng);

  /// query whether `number` is in the first set.
  bool in_first(size_t number) const;

  /// Swap the indices idx_first, idx_second
  /// The indices refer to the current (unsorted) internal list of the elements
  /// in the first and second set.
  void swap_indices(size_t idx_first, size_t idx_second);

  /// Render the two sets
  utils::Structure render() const override;

  void copy_state_only(const Partition& other) { in_first_ = other.in_first_; }

 private:
  std::vector<bool> in_first_;
  std::vector<size_t> first_;
  std::vector<size_t> second_;
};

}  // namespace model
