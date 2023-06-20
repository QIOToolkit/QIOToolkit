// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include <cmath>
#include <limits>

#include "utils/range.h"
#include "solver/parameter_free_adapter.h"
#include "solver/substochastic_monte_carlo.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// SSMCParameterFree is the class which implements
/// ParameterFreeAdapterInterface to fit in parameter free solver framework. And
/// it is a child of SubstochasticMonteCarlo so that it immediately has the
/// functionalities of SSMC
///
template <class Model_T>
class SSMCParameterFree : public SubstochasticMonteCarlo<Model_T>,
                          public ParameterFreeAdapterInterface
{
 public:
  SSMCParameterFree()
      : initial_counter_(0),
        max_replica_mult_(3),
        max_step_divider_(40.),
        initial_sweeps_(20),
        initial_steps_per_walker_(1),
        initial_alpha_final_(0.00001),
        initial_alpha_scope_(0.8),
        initial_beta_initial_(0.00001),
        initial_beta_scope_(5),
        max_steps_(std::numeric_limits<uint16_t>::max()),
        population_start_(0),
        population_end_(0),
        reserved_population_(0),
        steps_range_(1., 5.),
        alpha_final_range_(0.00001, 0.2),
        alpha_scope_range_(0.8, 1.0),
        beta_initial_range_(0.00001, 8.0),
        beta_scope_range_(1, 8.0)
  {
  }
  SSMCParameterFree(const SSMCParameterFree&) = delete;
  SSMCParameterFree& operator=(const SSMCParameterFree&) = delete;

  void configure(const utils::Json& json) override
  {
    SubstochasticMonteCarlo<Model_T>::configure(json);
    using ::matcher::GreaterThan;
    auto& params = json[utils::kParams];
    this->param(params, "steps_range", steps_range_)
        .description("steps per walker search range");
    this->param(params, "alpha_final_range", alpha_final_range_)
        .description("search range of alpha's final field");
    this->param(params, "alpha_scope_range", alpha_scope_range_)
        .description("search range of alpha's value scope");
    this->param(params, "beta_initial_range", beta_initial_range_)
        .description("search range of beta's initial value");
    this->param(params, "beta_scope_range", beta_scope_range_)
        .description("search range of beta's value scope");
    this->param(params, "max_replica_mult", max_replica_mult_)
        .matches(GreaterThan<size_t>(0))
        .description("multiplier for max number of replicas");
    this->param(params, "max_step_divider", max_step_divider_)
        .matches(GreaterThan<double>(1.0))
        .description("divider for setting up the step range");

    this->param(params, "initial_sweeps", initial_sweeps_)
        .matches(GreaterThan<uint32_t>(0))
        .description("initial number of sweeps to train the model");
    this->param(params, "initial_steps_per_walker", initial_steps_per_walker_)
        .matches(GreaterThan<uint32_t>(0))
        .description("initial steps per walker");
    this->param(params, "initial_alpha_final", initial_alpha_final_)
        .matches(GreaterThan<double>(0))
        .description("initial alpha final");
    this->param(params, "initial_alpha_scope", initial_alpha_scope_)
        .matches(GreaterThan<double>(0))
        .description("initial alpha scope");
    this->param(params, "initial_beta_initial", initial_beta_initial_)
        .matches(GreaterThan<double>(0))
        .description("initial beta initial value");
    this->param(params, "initial_beta_scope", initial_beta_scope_)
        .matches(GreaterThan<double>(0))
        .description("initial beta scope value");

    this->param(params, "reserved_population", reserved_population_)
        .description(
            "number of best replica reserved between execution iteration");

    double limit = std::sqrt(this->get_model_sweep_size()) *
                   this->get_model_term_size() * this->get_model_sweep_size();
    if (limit < std::numeric_limits<uint32_t>::max())
    {
      max_steps_ = std::max(max_steps_, (size_t)limit);
    }
    else
    {
      max_steps_ = std::numeric_limits<uint32_t>::max();
    }

    this->param(params, "max_steps", max_steps_)
        .matches(GreaterThan<uint64_t>(std::numeric_limits<uint16_t>::max()))
        .description("Max steps allowed");
    if (limit < max_steps_)
    {
      LOG(WARN, "maximum steps is:", max_steps_);
    }
    population_end_ = this->max_replicas_adjusted_state(max_replica_mult_);
    population_start_ = (double)this->thread_count_;

    assert(population_end_ >= 1);
    if (population_start_ >= population_end_)
    {
      population_start_ = population_end_ - 1.;
    }
  }

  size_t parameter_dimensions() const override { return 7; }
  std::vector<std::pair<double, double>> parameter_ranges() const override
  {
    // estimating the timing used by one step.
    double seconds_per_step = this->seconds_per_step();
    // seconds_per_step is just an estimation, so we divided by
    // max_step_divider_. to avoid use up all the time in one iteration
    double max_steps =
        this->time_limit_.value() / (seconds_per_step * max_step_divider_);
    size_t sweep_size = this->get_model_sweep_size();

    // ensure the max_steps is not too small
    max_steps = std::max(max_steps, (double)100);
    max_steps = std::max(max_steps, initial_sweeps_ * sweep_size + 1.0);
    max_steps = std::min((double)max_steps_, max_steps);

    LOG(INFO, "seconds_per_step:", seconds_per_step, ", max_steps:", max_steps);

    std::vector<std::pair<double, double>> ranges = {
        std::make_pair((double)initial_sweeps_ * sweep_size,
                       max_steps),  // this is for step_limit_
        std::make_pair(
            population_start_,
            population_end_),  // This is for range of target_population_
        std::make_pair(
            steps_range_.initial_,
            steps_range_.final_),  // This is for range of steps_per_walker_
        std::make_pair(
            alpha_final_range_.initial_,
            alpha_final_range_.final_),  // This is for range of final of alpha_
        std::make_pair(
            alpha_scope_range_.initial_,
            alpha_scope_range_.final_),  // This is for range of scope of alpha_
        std::make_pair(beta_initial_range_.initial_,
                       beta_initial_range_
                           .final_),  // This is for range of initial of beta_
        std::make_pair(
            beta_scope_range_.initial_,
            beta_scope_range_.final_)  // This is for range of scope of beta_
    };
    assert(ranges.size() == parameter_dimensions());
    return ranges;
  }

  void get_initial_parameter_values(std::vector<double>& initials) override
  {
    size_t sweep_size = this->get_model_sweep_size();
    initials.push_back(
        (double)(initial_sweeps_ * sweep_size * (1 + initial_counter_)));
    double population_target = (double)this->thread_count_ + initial_counter_;
    size_t max_states = this->max_replicas_adjusted_state(max_replica_mult_);
    population_target = std::min(population_target, (double)max_states);
    initials.push_back(population_target);
    initials.push_back((double)initial_steps_per_walker_);
    initials.push_back(initial_alpha_final_);
    initials.push_back(initial_alpha_scope_);
    initials.push_back(initial_beta_initial_);
    initials.push_back(initial_beta_scope_ + 1.0 * this->rng_->uniform());
    assert(initials.size() == parameter_dimensions());
    initial_counter_++;
  }

  void update_parameters(const std::vector<double>& parameters,
                         double left_over_time) override
  {
    this->step_limit_ =
        std::max((uint64_t)std::floor(parameters[0]), (uint64_t)2);
    assert(max_replica_mult_ > 0);
    this->target_population_ =
        this->adjust_states(parameters[1], max_replica_mult_);

    this->steps_per_walker_ = std::max((size_t)parameters[2], (size_t)1);
    this->alpha_.make_linear(parameters[3] + parameters[4], parameters[3]);
    this->alpha_.set_stop(this->step_limit_.value());
    this->beta_.make_linear(parameters[5], parameters[5] + parameters[6]);
    this->beta_.set_stop(this->step_limit_.value());

    this->set_output_parameter("step_limit", this->step_limit_);
    this->set_output_parameter("target_population", this->target_population_);
    this->set_output_parameter("steps_per_walker", this->steps_per_walker_);
    this->set_output_parameter("alpha", this->alpha_);
    this->set_output_parameter("beta", this->beta_);

    this->reset(left_over_time);

    if (reserved_population_ == 0)
    {
      this->population_.clear();
    }
    else if (!this->population_.empty())
    {
      // Reserve the best reserved_population from last run to next one.
      unsigned solutions_to_return = std::min(
          this->solutions_to_return_, (unsigned)this->population_.size());
      unsigned reserved_population =
          std::min(reserved_population_, (unsigned)this->population_.size());
      reserved_population = std::min(reserved_population,
                                     (unsigned)(this->target_population_ / 2));
      if (reserved_population > solutions_to_return)
      {
        this->population_.partial_sort(reserved_population);
      }
      this->population_.resize(reserved_population);
      this->prepare_population();
    }
  }

  void update_parameters_linearly(std::vector<double>& parameters) const override
  {
    parameters[0] = parameters[0] * 1.5;
  }

  double estimate_execution_cost() const override
  {
    size_t thread_count = this->get_thread_count();
    return (double)this->step_limit_.value() * this->steps_per_walker_ *
           ((this->target_population_ + thread_count - 1) / thread_count);
  }

 private:
  // Track the number of initial parameter set provided so far
  int initial_counter_;
  // Multiplier for setting the value range of population.
  uint32_t max_replica_mult_;
  // Divider for setting the max range of step_limit.
  double max_step_divider_;
  // Initial number of sweeps for setting step_limit of initial parameters
  uint32_t initial_sweeps_;
  // Initial steps_per_walker value
  uint32_t initial_steps_per_walker_;
  // Initial final value of alpha range
  double initial_alpha_final_;
  // Initial scope range of alpha, initial_alpha_scope_ + initial_alpha_final_
  // will be the initial of initial alpha.
  double initial_alpha_scope_;
  // Initial beta value's initial value
  double initial_beta_initial_;
  // Initial scope range of beta, initial_beta_initial_ + initial_beta_scope_
  // will be the final of initial beta
  double initial_beta_scope_;
  // max possible steps for search
  uint64_t max_steps_;

  // Start range value of population
  double population_start_;
  // End range value of population
  double population_end_;

  // Number of population reserved
  unsigned reserved_population_;

  // Range for steps per walker
  utils::Range steps_range_;
  // Range of alpha's final value
  utils::Range alpha_final_range_;
  // Range of alpha's scope value, alpha_scope + alpha_final will be the initial
  // of alpha
  utils::Range alpha_scope_range_;
  // Range of beta's initial value
  utils::Range beta_initial_range_;
  // Range of beta's scope value, beta_scope + beta_initial will be the final of
  // beta
  utils::Range beta_scope_range_;
};
}  // namespace solver
