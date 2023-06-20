// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include "markov/walker.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// A `Hill climbing walker` accepts every move reducing the cost and
/// keeps moving if cost is same.
/// NOTE: computing cost_difference is typically not superfluous as it will be
/// needed within `Walker::make_step()` to update the cached `cost_`.
template <class Model>
class HillClimbingWalker : public Walker<Model>
{
 public:
  bool accept(const typename Model::Cost_T& cost_diff) override
  {
    return cost_diff <= 0;
  }
};

}  // namespace markov
