// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "utils/random_generator.h"
#include "utils/structure.h"
#include "strategy/base_opt.h"
#include "strategy/gd_parameters.h"

namespace strategy
{
////////////////////////////////////////////////////////////////////////////////
/// Bayesian Global Optimization stategy searching implementation.
/// The BayesianOpt is a wrapper class for GPP library
/// https://github.com/wujian16/Cornell-MOE The paper "A Tutorial on Bayesian
/// Optimization" is a good introduction of bayesian optimization
///
class BayesianOpt : public BaseOpt
{
 public:
  BayesianOpt() : sample_cur_(0), new_sample_index_(0), reserved_samples_(64){};
  BayesianOpt(const BayesianOpt&) = delete;
  BayesianOpt& operator=(const BayesianOpt&) = delete;
  // Load the Bayesian Optimization parameters
  void configure(const utils::Json& params, int thread_count);
  // Initialize the Bayesian Optimization Process
  void init(size_t dimensions, uint32_t seed, size_t reserved_samples = 128);
  void init(size_t dimensions, uint32_t seed,
            const GDParameters& model_parameters,
            const GDParameters& search_parameters,
            size_t reserved_samples = 128);
  // Return next set of good parameters found by bayesian optimization process
  bool recommend_parameter_values(std::vector<double>& parameters_new) override;
  // Feedback a new set of parameters and objective value to learning process
  void add_new_sample(std::vector<double>& parameters,
                      double objective) override;
  // Set the parameter searching value ranges
  void set_ranges(const std::vector<std::pair<double, double>>& ranges);
  // Number of samples saved for training
  size_t num_of_saved_samples() const;
  double get_sample(int indx, std::vector<double>& sample_point) const;

  uint32_t get_reserved_samples() { return reserved_samples_; }

 private:
  // copy out the best parameter values found
  void copyout_winner(std::vector<double>& parameters) const;
  // logging the hypyer parameters of bayesian optimization
  void log_hyper_parameters() const;
  // Vector of sample points (X)
  std::vector<double> sample_points_;
  // Target function's objective value corresponsed to  sample_points_
  std::vector<double> sample_objectives_;
  // Best (lowest) objective value found so far.
  double best_so_far_;
  // Parameter search ranges.
  std::vector<std::pair<double, double>> ranges_;
  // Random number generator used
  ::utils::Twister rng_;
  // gradient descent search parameters for model selection
  GDParameters model_training_;

  // gradient descent search parameters for search process
  GDParameters search_training_;
  // index of current sample (parameters) used
  int sample_cur_;
  // Solver parameters found in last bayesian search process
  std::vector<double> next_winner_points_;
  // Hyper-parameters used for bayesian search process
  std::vector<double> trained_parameters_;
  // Index of new samples to be inserted in sample_points_
  size_t new_sample_index_;
  // Max number of samples (in unit of dimension) saved in sample_points_ for
  // model training
  size_t reserved_samples_;
};
}  // namespace strategy