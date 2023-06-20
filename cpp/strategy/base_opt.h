// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "utils/component.h"
#include "utils/random_generator.h"
#include "utils/structure.h"
namespace strategy
{
////////////////////////////////////////////////////////////////////////////////
/// Global Optimization stategy searching implementation.
class BaseOpt : public utils::Component
{
 public:
  BaseOpt() : dimensions_(0), learning_time_ms_(0), model_time_ms_(0){};
  BaseOpt(const BaseOpt&) = delete;
  BaseOpt& operator=(const BaseOpt&) = delete;
  // Return next set of good parameters found by optimization process
  virtual bool recommend_parameter_values(
      std::vector<double>& parameters_new) = 0;
  // Feedback a new set of parameters and objective value to learning process
  virtual void add_new_sample(std::vector<double>& parameters,
                              double objective) = 0;
  // Retrieve the performance telemetry related with the optimization search
  void get_perf_metrics(utils::Structure& perf) const;
#ifdef qiotoolkit_PROFILING
  // add the performance profiling numbers to s
  void add_profiling(utils::Structure& s) const;
#endif
 protected:
  // logging the parameters values
  static void log_parameters(const std::string& header,
                             const std::vector<double>& parameters);
  // Parameter searching dimensions
  size_t dimensions_;
  // Parameter search ranges.
  std::vector<std::pair<double, double>> ranges_;
  // parameter searching time in miliseconds
  int64_t learning_time_ms_;
  // Gaussian model building time in miliseconds
  int64_t model_time_ms_;
#ifdef qiotoolkit_PROFILING
  // updating the information for profiling
  void update_profile(int64_t model_ms, int64_t learning_ms);
  // update the objective and parameter values
  void update_objective_profile(double objective,
                                const std::vector<double>& parameters);
  // sample points saved for profiling output
  std::vector<double> profile_sample_points_;
  // target function's objectives corresponded to profile_sample_points_
  std::vector<double> profile_sample_objectives_;
  // model trainning time in miliseconds saved for profiling
  std::vector<int64_t> profile_modeling_ms_;
  // parameter searching time in miliseconds saved for profiling
  std::vector<int64_t> profile_learning_ms_;
#endif
};
}  // namespace strategy