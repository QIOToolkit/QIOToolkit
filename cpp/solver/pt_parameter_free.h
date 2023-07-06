
#pragma once
#include <cmath>

#include "utils/exception.h"
#include "utils/range.h"
#include "solver/parallel_tempering.h"
#include "solver/parameter_free_adapter.h"
#include "schedule/schedule.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// PTParameterFree is the class which implements
/// ParameterFreeAdapterInterface to fit in parameter free solver framework. And
/// it is a child of ParallelTempering so that it immediately has the
/// functionalities of PT
///
template <class Model_T>
class PTParameterFree : public ParallelTempering<Model_T>,
                        public ParameterFreeLinearSearchAdapterInterface
{
 public:
  PTParameterFree()
      : max_prob_(0.5),     // Highest acceptance bound to set beta start
        min_prob_(0.0001),  // Min probability of cost increase to set beta stop
        min_cost_diff_adjustment_factor_(0.001),  // Factor for adjustment of minimum 
                                                  // cost difference estimation
        beta_start_(0.01),
        beta_stop_(10.0),
        min_num_betas_(16),  // Min number of replicas and betas
        tempering_size_increment_(5),  // Increment between sizes of increasing 
                                       // betas sequences
        initial_sweeps_sample_points_(
            {10, 20, 30, 50})  // Vector of initial sample points
                               // for finding optimal sweeps number
  {
  }
  PTParameterFree(const PTParameterFree&) = delete;
  PTParameterFree& operator=(const PTParameterFree&) = delete;

  void configure(const utils::Json& json) override
  {
    ParallelTempering<Model_T>::configure(json);

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
    this->param(params, "min_num_betas", min_num_betas_)
        .matches((GreaterThan(0)))
        .default_value(this->get_thread_count())
        .description("Min number of betas.");

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

    this->step_limit_ = initial_sweeps_sample_points_[0];
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
    size_t sweeps = std::lround(parameters[0]);
    size_t num_betas = min_num_betas_;
    // Set paramenters for parallel tempering
    this->step_limit_ = std::max(sweeps, (size_t)(2));
    std::vector<double> betas;
    determine_all_betas_(beta_start_, beta_stop_, num_betas, betas);
    this->temperatures_.make_list(betas);
    this->use_inverse_temperatures_ = true;
    this->set_output_parameter("all_betas", this->temperatures_);
    this->set_output_parameter("sweeps", this->step_limit_);
    this->reset(left_over_time);
  }

  double estimate_execution_cost() const override
  {
    size_t thread_count = this->get_thread_count();
    return double(this->step_limit_.value()) *
           double(this->target_number_of_states()) / double(thread_count);
  }

 private:
  void determine_all_betas_(double lowest_beta, double highest_beta,
                            size_t num_betas, std::vector<double>& betas)
  {
    size_t tempering_size = tempering_size_increment_;
    betas.clear();
    if (num_betas == 0) return;
    betas.reserve(num_betas);
    schedule::Schedule schedule;
    schedule.make_geometric(lowest_beta, highest_beta);
    while (true)
    {
      size_t n = tempering_size - 1;
      schedule.set_stop(n - 1);
      for (size_t i = 0; i < n; ++i)
      {
        betas.push_back(schedule.get_value(i));
        if (betas.size() >= num_betas) return;
      }
      tempering_size += tempering_size_increment_;
    }
  }

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

  int min_num_betas_;

  size_t tempering_size_increment_;

  // Vector of initial sample points for sweeps
  std::vector<int> initial_sweeps_sample_points_;
};
}  // namespace solver