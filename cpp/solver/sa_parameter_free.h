// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include <cmath>

#include "utils/exception.h"
#include "solver/parameter_free_adapter.h"
#include "solver/simulated_annealing.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// SAParameterFree is the class which implements
/// ParameterFreeAdapterInterface to fit in parameter free solver framework. And
/// it is a child of SimulatedAnnealing so that it immediately has the
/// functionalities of SA
///
template <class Model_T>
class SAParameterFree : public SimulatedAnnealing<Model_T>,
                        public ParameterFreeLinearSearchAdapterInterface
{
 public:

  SAParameterFree()
      : max_prob_(0.5),
        min_prob_(0.0001),
        min_cost_diff_adjustment_factor_(0.001),
        beta_start_(0.01),
        beta_stop_(10.0),
        initial_sweeps_sample_points_({10, 20, 30, 50})
  {
  }
  SAParameterFree(const SAParameterFree&) = delete;
  SAParameterFree& operator=(const SAParameterFree&) = delete;

  void configure(const utils::Json& json) override
  {
    SimulatedAnnealing<Model_T>::configure(json);
    using ::matcher::GreaterEqual;
    using ::matcher::GreaterThan;
    using ::matcher::LessEqual;
    auto& params = json[utils::kParams];
    this->param(params, "max_prob", max_prob_)
        .matches(GreaterThan<double>(0))
        .matches(LessEqual<double>(1))
        .description("Highest acceptance bound to set beta start.");
    this->param(params, "min_prob", min_prob_)
        .matches(GreaterThan<double>(0))
        .matches(LessEqual<double>(1))
        .description("Min probability of cost increase to set beta stop.");
    this->param(params, "min_cost_diff_adjustment_factor",
                min_cost_diff_adjustment_factor_)
        .matches(GreaterThan<double>(0))
        .matches(LessEqual<double>(1))
        .description(
            "Factor for adjustment of minimum cost difference estimation.");
    this->param(params, "initial_sweeps_sample_points",
                initial_sweeps_sample_points_)
        .matches(SizeIs(GreaterThan(2)))
        .description("Vector of initial sample points for sweeps.");
    this->param(params, "min_num_restarts", min_num_restarts_)
        .matches((GreaterThan(0)))
        .default_value(this->get_thread_count())
        .description("Min number of restarts.");

    if (!this->is_empty())
    {
      max_cost_diff_ = this->model_->estimate_max_cost_diff();
      min_cost_diff_ = this->model_->estimate_min_cost_diff();

      // Checks for model consistency
      if (max_cost_diff_ <= 0)
      {
        THROW(utils::InconsistentValueException,
              "max cost difference should be > 0, "
              "found :",
              max_cost_diff_);
      }

      if (min_cost_diff_ <= 0)
      {
        THROW(utils::InconsistentValueException,
              "min cost difference should be > 0, "
              "found :",
              min_cost_diff_);
      }

      if (min_cost_diff_ > max_cost_diff_)
      {
        THROW(utils::InconsistentValueException,
              "min cost difference should be <= max cost difference, "
              "found :",
              min_cost_diff_, " > ", max_cost_diff_);
      }

      beta_start_ = -log(max_prob_) / max_cost_diff_;

      double min_cost_diff_adjusted =
          min_cost_diff_ +
          min_cost_diff_adjustment_factor_ * (max_cost_diff_ - min_cost_diff_);

      beta_stop_ = -log(min_prob_) / min_cost_diff_adjusted;
    }

    this->use_inverse_temperature_ = true;
    this->schedule_.make_linear(beta_start_, beta_stop_);
    this->restarts_ = min_num_restarts_;

    this->step_limit_ = initial_sweeps_sample_points_[0];
    this->schedule_.set_stop(std::max(1, (int)this->step_limit_.value() - 1));

    this->set_output_parameter("beta_start", beta_start_);
    this->set_output_parameter("beta_stop", beta_stop_);
    LOG(INFO, "beta start:", this->beta_start_,
        ", beta stop:", this->beta_stop_);

    this->set_output_parameter("restarts", this->restarts_);
    this->set_output_parameter("sweeps", this->step_limit_);
  }

  size_t parameter_dimensions() const override { return 2; }

  std::vector<int> parameter_ranges() const override
  {
    return initial_sweeps_sample_points_;
  }

  void update_parameters(const std::vector<double>& parameters,
                         double left_over_time) override
  {
    int sweeps = std::lround(parameters[0]);
    int sweeps_back = std::lround(parameters[1]);
    //Set paramenters for simulation annealing
    this->step_limit_ = std::max(sweeps, 2);
    this->restarts_ = min_num_restarts_ * sweeps_back / sweeps;
    
    //Set schedule
    this->schedule_.make_linear(beta_start_, beta_stop_);
    this->schedule_.set_stop(std::max(1, (int)this->step_limit_.value() - 1));
    
    this->set_output_parameter("restarts", this->restarts_);
    this->set_output_parameter("sweeps", this->step_limit_);

    this->reset(left_over_time);
  }

  double estimate_execution_cost() const override
  {
    size_t thread_count = this->get_thread_count();
    return double(this->step_limit_.value()) *
           double(this->target_number_of_states()) /
           double(thread_count);
  }

 private:

  // estimated maximum cost difference
  double max_cost_diff_;
  // estimated minimum cost difference
  double min_cost_diff_;
  // Max probability for rejection
  double max_prob_;
  // Min probability for rejection
  double min_prob_;
  // Factor for adjustment of minimum cost difference estimation
  double min_cost_diff_adjustment_factor_;

  double beta_start_;

  double beta_stop_;

  int min_num_restarts_;

  //Vector of initial sample points for sweeps
  std::vector<int> initial_sweeps_sample_points_;
};
}  // namespace solver