
#pragma once

#include "markov/walker.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// A `RandomWalker` accepts every move.
///
/// NOTE: computing cost_difference is typically not superfluous as it will be
/// needed within `Walker::make_step()` to update the cached `cost_`.
template <class Model>
class RandomWalker : public Walker<Model>
{
 public:
  bool accept(const typename Model::Cost_T&) override { return true; }
};

}  // namespace markov
