
#pragma once
#include <cmath>

#include "utils/exception.h"
#include "utils/range.h"
#include "solver/parameter_free_adapter.h"
#include "solver/population_annealing.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// PAParameterFree is the class which implements
/// ParameterFreeAdapterInterface to fit in parameter free solver framework. And
/// it is a child of PopulationAnnealing so that it immediately has the
/// functionalities of PA
///
template <class Model_T>
class PAParameterFree : public PopulationAnnealing<Model_T>,
                        public ParameterFreeAdapterInterface
{
 public:
  PAParameterFree()
      : max_step_divider_(160.),
        min_population_mult_(4),
        initial_counter_(0),
        initial_sweeps_(20),
        max_population_mult_(128),
        initial_alpha_(2),
        initial_min_prob_log_(2),
        max_prob_(0.5),
        alpha_range_(1.00000001, 3),
        sweeps_per_replica_range_(1, 4),
        final_min_prob_log_(15),
        max_steps_(std::numeric_limits<uint32_t>::max() / 8),
        population_start_(0),
        population_end_(0),
        reserved_population_(0),
        friction_tensor_constant_range_(0.00000001, 5),
        initial_culling_fraction_range_(0.00000001, 5),
        constant_culling_fraction_range_(0.00000001, 0.2),
        initial_friction_tensor_constant_(1),
        initial_initial_culling_fraction_(0.5),
        initial_constant_culling_fraction_(0.1)
  {
  }
  PAParameterFree(const PAParameterFree&) = delete;
  PAParameterFree& operator=(const PAParameterFree&) = delete;

  void configure(const utils::Json& json) override
  {
    PopulationAnnealing<Model_T>::configure(json);
    using ::matcher::GreaterEqual;
    using ::matcher::GreaterThan;
    using ::matcher::LessEqual;
    auto& params = json[utils::kParams];
    this->param(params, "max_step_divider", max_step_divider_)
        .matches(GreaterThan<double>(1.0))
        .description("divider for setting up the step range");
    this->param(params, "min_population_mult", min_population_mult_)
        .matches(GreaterThan<double>(0))
        .description("multiplier for initial value of population");
    this->param(params, "initial_sweeps", initial_sweeps_)
        .matches(GreaterThan<uint32_t>(0))
        .description("Initial sweeps for parameter free PA");
    this->param(params, "initial_alpha", initial_alpha_)
        .matches(GreaterThan<double>(1))
        .description("Initial value of alpha");
    this->param(params, "initial_min_prob_log", initial_min_prob_log_)
        .matches(GreaterEqual<double>(0))
        .description("Initial value of minimum probility");
    this->param(params, "max_prob", max_prob_)
        .matches(GreaterThan<double>(0))
        .matches(LessEqual<double>(1))
        .description("Initial value of beta's scope");

    this->param(params, "max_population_mult", max_population_mult_)
        .matches(GreaterThan<uint32_t>(((uint32_t)min_population_mult_) + 1))
        .description("multiplier for max number of replicas");

    this->param(params, "alpha_range", alpha_range_)
        .description("Parameter alpha's value range");

    this->param(params, "sweeps_per_replica_range", sweeps_per_replica_range_)
        .description("Parameter sweeps per replica's value range");

    this->param(params, "friction_tensor_constant_range",
                friction_tensor_constant_range_)
        .description("Value range of friction_tensor_constant");
    this->param(params, "initial_culling_fraction_range",
                initial_culling_fraction_range_)
        .description("Value range of initial_culling_fraction_constant");
    this->param(params, "constant_culling_fraction_range",
                constant_culling_fraction_range_)
        .description("Value range of constant_culling_fraction");

    this->param(params, "initial_friction_tensor_constant",
                initial_friction_tensor_constant_)
        .matches(GreaterThan<double>(0.1))
        .description("Initial value of friction_tensor_constant");
    this->param(params, "initial_initial_culling_fraction",
                initial_initial_culling_fraction_)
        .matches(GreaterThan<double>(0.1))
        .description("Value range of initial_culling_fraction");
    this->param(params, "initial_constant_culling_fraction",
                initial_constant_culling_fraction_)
        .matches(GreaterThan<double>(0.01))
        .description("Value range of constant_culling_fraction");

    this->param(params, "reserved_population", reserved_population_)
        .description(
            "number of best replica reserved between execution iteration");

    double limit =
        std::sqrt(this->get_model_sweep_size()) * this->get_model_term_size();
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

    min_cost_diff_ = this->model_->estimate_min_cost_diff();
    max_cost_diff_ = this->model_->estimate_max_cost_diff();
    LOG(INFO, "min cost diff:", min_cost_diff_,
        ", max cost diff:", max_cost_diff_);
    this->beta_start_ = -std::log(max_prob_) / max_cost_diff_;
    start_min_prob_log_ = min_cost_diff_ * this->beta_start_ * 2;
    this->param(params, "final_min_prob_log", final_min_prob_log_)
        .matches(GreaterThan<double>(start_min_prob_log_))
        .description("Negative log of min prob value's range");
    start_min_prob_log_ = std::max(initial_min_prob_log_, start_min_prob_log_);
    final_min_prob_log_ =
        std::max(final_min_prob_log_, start_min_prob_log_ * 5);
    LOG(INFO, "start min prob log value:", start_min_prob_log_,
        ", final min prob log value:", final_min_prob_log_);
    this->set_output_parameter("beta_start", this->beta_start_);

    population_end_ = this->max_replicas_adjusted_state(max_population_mult_);
    population_start_ = (double)this->thread_count_ * min_population_mult_;

    assert(population_end_ >= 1);
    if (population_start_ >= population_end_)
    {
      population_start_ = population_end_ - 1.;
    }
  }

  void update_parameters_linearly(std::vector<double>& parameters) const override
  {
    // increase the sweeps by 1.5 times, 1.5 is a magic number from our old
    // experince.
    parameters[0] = parameters[0] * 1.5;
  }

  size_t parameter_dimensions() const override { return 5; }

  std::vector<std::pair<double, double>> parameter_ranges() const override
  {
    // estimating the timing used by one step. seconds_per_step is just an
    // estimation.
    double seconds_per_step = this->seconds_per_step();
    double max_steps =
        this->time_limit_.value() / (seconds_per_step * max_step_divider_);

    // ensure the max_steps is not too small
    max_steps = std::max(max_steps, (double)initial_sweeps_ + 1);
    max_steps = std::min((double)max_steps_, max_steps);

    std::vector<std::pair<double, double>> ranges = {
        std::make_pair((double)initial_sweeps_,
                       max_steps),  // This is for step_limit_
        std::make_pair(population_start_,
                       population_end_),  // This is for range of population_
        std::make_pair(alpha_range_.initial_,
                       alpha_range_.final_),  // This is for range of alpha
        std::make_pair(sweeps_per_replica_range_.initial_,
                       sweeps_per_replica_range_
                           .final_)  // This is for range of sweeps per replica
    };
    if (this->is_linear_schedule())
    {
      ranges.push_back(std::make_pair(
          start_min_prob_log_,
          final_min_prob_log_));  // This is for negative log of minimum
                                  // acceptance probability range.
    }
    else
    {
      // The following code are ranges of other PA strategy which has not been
      // tuned yet.
      if (this->is_friction_tensor())
      {
        ranges.push_back(
            std::make_pair(friction_tensor_constant_range_.initial_,
                           friction_tensor_constant_range_.final_));
      }
      else if (this->is_energy_variance())
      {
        ranges.push_back(
            std::make_pair(initial_culling_fraction_range_.initial_,
                           initial_culling_fraction_range_.final_));
      }
      else if (this->is_constant_culling())
      {
        ranges.push_back(
            std::make_pair(constant_culling_fraction_range_.initial_,
                           constant_culling_fraction_range_.final_));
      }
    }

#ifdef qiotoolkit_PROFILING
    LOG(INFO, ": population initial:", population_start_,
        " population final:", population_end_, " sweeps end:", max_steps);
#endif
    assert(ranges.size() == parameter_dimensions());
    return ranges;
  }

  void get_initial_parameter_values(std::vector<double>& initials) override
  {
    initials.push_back((double)(initial_sweeps_ + initial_counter_ * 10));
    double population = (double)this->thread_count_ * (initial_counter_ + 1);
    population = this->adjust_states(population, max_population_mult_);
    initials.push_back(population);
    initials.push_back(initial_alpha_ + 0.5 * (this->rng_->uniform() - 0.5));
    initials.push_back(initial_counter_ + 1.0);

    if (this->is_linear_schedule())
    {
      initials.push_back(initial_min_prob_log_ + initial_counter_ * 2);
    }
    else
    {
      if (this->is_friction_tensor())
      {
        initials.push_back(initial_friction_tensor_constant_ +
                           0.1 * (this->rng_->uniform() - 0.5));
      }
      else if (this->is_energy_variance())
      {
        initials.push_back(initial_initial_culling_fraction_ +
                           0.1 * (this->rng_->uniform() - 0.5));
      }
      else if (this->is_constant_culling())
      {
        initials.push_back(initial_constant_culling_fraction_ +
                           0.01 * (this->rng_->uniform() - 0.5));
      }
    }

    assert(initials.size() == parameter_dimensions());
#ifdef qiotoolkit_PROFILING
    LOG(INFO, initial_counter_, ": initial sweeps:", initials[0],
        " initial populations:", initials[1]);
#endif
    initial_counter_++;
  }

  void update_parameters(const std::vector<double>& parameters,
                         double left_over_time) override
  {
    // this->sweeps_per_replica_ shall be at least 1
    this->sweeps_per_replica_ =
        std::max((size_t)1, (size_t)std::lround(parameters[3]));
    this->set_output_parameter("sweeps_per_replica", this->sweeps_per_replica_);

    // need to reset the time limit, at least 2 steps
    this->step_limit_ =
        std::max((uint64_t)std::ceil(parameters[0] / this->sweeps_per_replica_),
                 (uint64_t)2);

    assert(max_population_mult_ > 0);
    this->target_population_ =
        this->adjust_states(parameters[1], max_population_mult_);

    this->alpha_ = std::max(parameters[2], alpha_range_.initial_);
    if (this->is_linear_schedule())
    {
      this->beta_stop_ = (parameters[4] / min_cost_diff_);
      assert(this->beta_stop_ > this->beta_start_);
      this->beta_.make_geometric(this->beta_start_, this->beta_stop_);
      this->set_output_parameter("beta", this->beta_);
      this->set_output_parameter("beta_stop", this->beta_stop_);
      this->beta_.set_stop(std::max(1, (int)this->step_limit_.value() - 1));
      LOG(INFO, "beta start:", this->beta_start_,
          ", beta stop:", this->beta_stop_);
    }
    else
    {
      this->beta_start_ = 0;
      if (this->is_friction_tensor())
      {
        this->friction_tensor_constant_ = parameters[4];
        this->set_output_parameter("friction_tensor_constant",
                                   this->friction_tensor_constant_);
      }
      else if (this->is_energy_variance())
      {
        this->initial_culling_fraction_ = parameters[4];
        this->set_output_parameter("initial_culling_fraction",
                                   this->initial_culling_fraction_);
      }
      else if (this->is_constant_culling())
      {
        this->constant_culling_fraction_ = parameters[4];
        this->set_output_parameter("culling_fraction",
                                   this->constant_culling_fraction_);
      }
    }

    this->set_output_parameter("sweeps", this->step_limit_);
    this->set_output_parameter("population", this->target_population_);
    this->set_output_parameter("alpha", this->alpha_);

#ifdef qiotoolkit_PROFILING
    LOG(INFO, "sweeps:", this->step_limit_.value(),
        " population:", this->target_population_);
#endif
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
      this->expand_population();
    }
  }

  double estimate_execution_cost() const override
  {
    size_t thread_count = this->get_thread_count();
    return (double)this->step_limit_.value() * this->sweeps_per_replica_ *
           ((this->target_population_ + thread_count - 1) / thread_count);
  }

 private:
  // Divider for setting the max range of step_limit.
  double max_step_divider_;
  // min multiplier for lower bound of population search range
  double min_population_mult_;
  // count the number of initial samples call
  int initial_counter_;
  // initial number of sweeps for training the search process
  uint32_t initial_sweeps_;
  // max multiplier for upper bound of population search range.
  uint32_t max_population_mult_;
  // initial alpha value for training the search process
  double initial_alpha_;
  // Min probability for rejection
  double initial_min_prob_log_;
  // Max probability for rejection
  double max_prob_;
  // alpaha value range.
  utils::Range alpha_range_;
  // sweeps_per_replica range value
  utils::Range sweeps_per_replica_range_;
  // upper bound value of minimum rejection probability's negative log value
  double final_min_prob_log_;
  // lower bound value of minimum rejection probability's negative log value,
  // this value is calculated by start_min_prob_log_ = min_cost_diff_ *
  // this->beta_start_ * 2;
  double start_min_prob_log_;
  // Max steps allowed for searching.
  uint64_t max_steps_;

  // Start value of population range
  double population_start_;
  // End value of population range
  double population_end_;
  // Number of population reserved
  unsigned reserved_population_;

  /// The following are for experimental resampling strategy
  // value range of friction_tensor_constant
  utils::Range friction_tensor_constant_range_;
  // value range of initial_culling_fraction
  utils::Range initial_culling_fraction_range_;
  // value range of constant_culling_fraction
  utils::Range constant_culling_fraction_range_;
  // initial value of friction_tensor_constant
  double initial_friction_tensor_constant_;
  // initial value of initial_culling_fraction
  double initial_initial_culling_fraction_;
  // initial value of constant_culling_fraction
  double initial_constant_culling_fraction_;
  // estimated minimum cost difference
  double min_cost_diff_;
  // estimated maximum cost difference
  double max_cost_diff_;
};
}  // namespace solver