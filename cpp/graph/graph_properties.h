// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

namespace graph
{
struct GraphProperties
{
  GraphProperties()
      : allow_dup_merge_(false),
        max_locality_(0),
        min_locality_(UINT_MAX),
        avg_locality_(0),
        accumulated_dependent_vars_(0),
        accumulated_dependent_terms_(0),
        max_coupling_magnitude_(0),
        min_coupling_magnitude_(DBL_MAX),
        total_locality_(0),
        const_cost_(0),
        scale_factor_(1.0),
        is_rescaled_(false)
  {
  }

  void rescale()
  {
    if (is_rescaled_)
    {
      throw utils::ValueException(
          "rescale has been called, recale can only be done once!");
    }
    is_rescaled_ = true;

    if (max_coupling_magnitude_ <= 1 || max_coupling_magnitude_ == 0)
    {
      // max_coupling_magnitude_ == 0, means all terms are constant 0
      // and we want to reduce, not increase the coefficient
      return;
    }
    scale_factor_ = 1.0 / max_coupling_magnitude_;
  }

  bool allow_dup_merge_;

  uint32_t max_locality_;
  uint32_t min_locality_;
  double avg_locality_;
  uint64_t accumulated_dependent_vars_;
  uint64_t accumulated_dependent_terms_;
  double max_coupling_magnitude_;
  double min_coupling_magnitude_;
  uint64_t total_locality_;
  double const_cost_;

  double scale_factor_;
  bool is_rescaled_;
};
}  // namespace graph