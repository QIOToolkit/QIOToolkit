// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "utils/exception.h"
#include "utils/operating_system.h"
#include "markov/metropolis.h"
#include "omp.h"
#include "schedule/schedule.h"
#include "solver/stepping_solver.h"

namespace solver
{
template <class Model_T>
class SimulatedAnnealing : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  SimulatedAnnealing() : use_inverse_temperature_(false), restarts_(1) {}

  SimulatedAnnealing(const SimulatedAnnealing&) = delete;
  SimulatedAnnealing& operator=(const SimulatedAnnealing&) = delete;

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override
  {
    return "microsoft.simulatedannealing.qiotoolkit";
  }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too many restarts, terms and/or "
        "variables). "
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override { return restarts_; }

  void init() override
  {
    this->init_memory_check();
    // proceed to memory allocation

    replicas_.resize(restarts_);
    rngs_.resize(restarts_);

    for (size_t i = 0; i < restarts_; i++)
    {
      rngs_[i] = this->rng_->fork();
    }
    utils::OmpCatch omp_catch;
    #pragma omp parallel for
    for (size_t i = 0; i < restarts_; i++)
    {
      omp_catch.run([=] {
        replicas_[i].set_model(this->model_);
        replicas_[i].set_rng(rngs_[i].get());
        replicas_[i].reset_evaluation_counter();
        replicas_[i].init();
      });
    }
    omp_catch.rethrow();
    for (size_t i = 0; i < restarts_; i++)
    {
      this->evaluation_counter_ += replicas_[i].get_evaluation_counter();
    }
    this->update_lowest_cost(replicas_[0].cost(), replicas_[0].state());
  }

  void make_step(uint64_t step) override
  {
    auto temperatureorbeta = schedule_.get_value((double)step);
    #pragma omp parallel for
    for (size_t i = 0; i < restarts_; i++)
    {
      replicas_[i].reset_evaluation_counter();
      if (use_inverse_temperature_)
      {
        replicas_[i].set_beta(temperatureorbeta);
      }
      else
      {
        replicas_[i].set_temperature(temperatureorbeta);
      }
      replicas_[i].make_sweep();
    }

    if (this->eval_limit_.has_value())
    {
      for (size_t i = 0; i < restarts_; i++)
      {
        this->evaluation_counter_ += replicas_[i].get_evaluation_counter();
      }
    }

    if (this->cost_limit_.has_value())
    {
      for (const auto& replica : replicas_)
      {
        this->update_lowest_cost(replica.get_lowest_cost(),
                                 replica.get_lowest_state());
      }
    }
  }

  utils::Structure get_solutions() const override
  {
#ifdef _DEBUG
    double recal_cost = this->model_->calculate_cost(this->lowest_state_);
    DEBUG("recorded cost:", std::fixed, *this->lowest_cost_);
    DEBUG("calculated_cost:", std::fixed, recal_cost);
    assert(abs(recal_cost - *this->lowest_cost_) < 0.1);
#endif
    return Base_T::get_solutions();
  }

  void configure(const utils::Json& json) override
  {
    Base_T::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kParams))
    {
      THROW(utils::MissingInputException, "Input field `", utils::kParams,
            "` is required.");
    }
    const utils::Json& params = json[utils::kParams];

    this->param(params, "use_inverse_temperature", use_inverse_temperature_)
        .description("set beta=1/T from the schedule, rather than T itself.")
        .with_output()  // only output if explicitly set.
        .default_value(false);

    bool using_qio_format = false;
    if (params.HasMember("schedule"))
    {
      this->param(params, "schedule", schedule_)
          .description("annealing schedule")
          .required()
          .with_output();
    }
    else
    {
      // For compatibility with microsoft.simulatedannealing.cpu, allow the
      // annealing schedule to be specified with beta_low, beta_high.
      using ::matcher::GreaterEqual;
      using ::matcher::GreaterThan;
      double beta_start, beta_stop;
      this->param(params, "beta_start", beta_start)
          .description("Starting inverse temperature for annealing")
          .matches(GreaterEqual(0.0))
          .default_value(0)
          .with_output();
      this->param(params, "beta_stop", beta_stop)
          .description("Final inverse temperature for annealing")
          .matches(GreaterThan(beta_start))
          .default_value(0.01)
          .with_output();
      use_inverse_temperature_ = true;
      schedule_.make_linear(beta_start, beta_stop);
      using_qio_format = true;
    }

    // If the schedule does not inherently have a range (start & stop) or
    // count (intended number of steps), stretch the schedule to the number
    // of steps given by step_limit.
    //
    // NOTE: The effect of this is that simulated annealing will do a single
    // descent over the number of steps the simulation will run.
    //
    if (schedule_.get_start() == 0 && schedule_.get_stop() == 1 &&
        schedule_.get_count() == 1)
    {
      if (this->step_limit_.has_value())
      {
        if (this->step_limit_.value() > 1)
        {
          if (!using_qio_format)
          {
            LOG(INFO, "Stretching simulated_annealing.schedule to ",
                this->step_limit_.value(), " steps. You can change this by ",
                "explicitly setting schedule.{start,stop} or schedule.count.");
          }
          schedule_.set_stop(std::max(1, (int)this->step_limit_.value() - 1));
        }
        else
        {
          throw(utils::ValueException(
              "The number of sweeps must be greater than 1."));
        }
      }
      else
      {
        throw(utils::MissingInputException(
            "Either a step_limit or schedule.count must be provided. "
            "You can also set schedule.{start,stop} to define the range."));
      }
    }

    this->param(params, "restarts", restarts_)
        .description("number of restarts for simulated annealing")
        .default_value(static_cast<size_t>(this->thread_count_))
        .with_output()
        .matches(::matcher::GreaterThan<size_t>(0));
  }

  bool use_inverse_temperature() const { return use_inverse_temperature_; }

  void finalize() override { this->populate_solutions(replicas_); }

 protected:
  bool use_inverse_temperature_;
  size_t restarts_;
  ::schedule::Schedule schedule_;

 private:
  std::vector<std::unique_ptr<::utils::RandomGenerator>> rngs_;
  std::vector<::markov::Metropolis<Model_T>> replicas_;
};
REGISTER_SOLVER(SimulatedAnnealing);

}  // namespace solver
