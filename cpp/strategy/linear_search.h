// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include <memory>
#include <utility>
#include <vector>
#include "utils/random_generator.h"
#include "utils/structure.h"
#include "strategy/base_opt.h"

namespace strategy
{
////////////////////////////////////////////////////////////////////////////////
/// Linear Search Global Optimization stategy searching implementation.
/// Implemented 1 dimensional search and refine algorithm.
/// Solution is computed at sample points iteratively and 
/// search interval defined by sample points will be moved  
/// toward the end where the best solution is observed

class LinearSearchOpt : public BaseOpt
{
 public:
  LinearSearchOpt() : sample_cur_(-1), search_interval_change_factor_(1.5) {}
  LinearSearchOpt(const LinearSearchOpt&) = delete;
  LinearSearchOpt& operator=(const LinearSearchOpt&) = delete;
  void configure(const utils::Json& params, int thread_count);
  void init(size_t dimensions, uint32_t seed, size_t reserved_samples = 128);

  // Return next set of good parameters found by bayesian optimization process
  bool recommend_parameter_values(std::vector<double>& parameters_new);

  // Feedback a new set of parameters and objective value to learning process
  void add_new_sample(std::vector<double>& parameters,
                                     double objective);

  // Set initial sample points in the range of the search parameter
  void set_ranges(const std::vector<int>& intial_sample_points);

 protected:
 /// Search interval defined by sample points will be stretched
 /// if the best solution is produced at the last sample point.  
 /// Search interval defined by sample points will be shrinked
 /// if the best solution is produced at the first sample point.  
  void fit_sample_points();
  std::vector<int> sample_points_;
  // index of current sample (parameters) used
  int sample_cur_;
  std::vector<double> sample_objectives_;
  double search_interval_change_factor_;
  int initial_minimum_sample_point_;
};
}  // namespace strategy