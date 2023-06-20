// Copyright (c) Microsoft. All Rights Reserved.
#include "model/partition.h"

namespace model
{
Partition Partition::random(size_t N, size_t K, utils::RandomGenerator& rng)
{
  Partition p;
  p.in_first_.resize(N);
  for (size_t i = 0; i < K; i++)
  {
    p.in_first_[i] = true;
  }
  // DO_NOT_SUBMIT
  std::shuffle(p.in_first_.begin(), p.in_first_.end(), rng);
  for (size_t i = 0; i < N; i++)
  {
    (p.in_first_[i] ? p.first_ : p.second_).push_back(i);
  }
  return p;
}

bool Partition::in_first(size_t number) const { return in_first_[number]; }

void Partition::swap_indices(size_t idx_first, size_t idx_second)
{
  std::swap(first_[idx_first], second_[idx_second]);
  in_first_[first_[idx_first]] = true;
  in_first_[second_[idx_second]] = false;
}

utils::Structure Partition::render() const
{
  utils::Structure s;
  // Don't copy first_, second_ directly, we want the rendered
  // output to be sorted.
  for (size_t i = 0; i < in_first_.size(); i++)
  {
    (in_first_[i] ? s["first"] : s["second"]).push_back(i);
  }
  return s;
}

}  // namespace model
