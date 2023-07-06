
#pragma once

#include <algorithm>
#include <climits>
#include <vector>

#include "utils/exception.h"
#include "utils/operating_system.h"
#include "markov/metropolis.h"
#include "observe/observer.h"
#include "omp.h"
#include "schedule/schedule.h"
#include "solver/stepping_solver.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// Parallel Tempering algorithm
///
/// Run multiple metropolis simulating at separate temperatures, allowing
/// exchange of configuration between neighboring replicas between sweeps.
///
/// NOTE: Use of the Metropolis Algorithm is currently hard-coded, but this
/// could in principle be a template argument, provided that the alternative
/// method implements the corresponding interfaces for exchange moves.
///
template <class Model_T>
class ParallelTempering : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  /// Create an uninitialized Parallel Tempering instance
  ParallelTempering() : use_inverse_temperatures_(false) {}

  ParallelTempering(const ParallelTempering&) = delete;
  ParallelTempering& operator=(const ParallelTempering&) = delete;

  std::vector<double> get_temperatures()
  {
    return temperatures_.get_discretized_values();
  }

  bool use_inverse_temperatures() { return use_inverse_temperatures_; }

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override
  {
    return "paralleltempering.qiotoolkit";
  }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too many replicas, terms and/or "
        "variables)."
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override
  {
    return temperatures_.get_discretized_values().size();
  }

  /// Initialize each replica
  void init() override
  {
    // Prepare the temperature set
    auto temperatures = temperatures_.get_discretized_values();
    size_t N = temperatures.size();
    if (use_inverse_temperatures_)
    {
      std::sort(temperatures.begin(), temperatures.end(),
                std::greater<double>());
    }
    else
    {
      std::sort(temperatures.begin(), temperatures.end(), std::less<double>());
    }

    this->init_memory_check();
    // proceed to memory allocation

    // Prepare the replicas
    replicas_.resize(N);
    direction_.resize(N);
    rngs_.resize(N);
    for (size_t i = 0; i < N; i++)
    {
      rngs_[i] = this->rng_->fork();
    }

    #pragma omp parallel for
    for (size_t i = 0; i < N; i++)
    {
      replicas_[i].set_model(this->model_);
      replicas_[i].set_rng(rngs_[i].get());
      replicas_[i].reset_evaluation_counter();
      if (use_inverse_temperatures_)
      {
        replicas_[i].set_beta(temperatures[i]);
      }
      else
      {
        replicas_[i].set_temperature(temperatures[i]);
      }
      direction_[i] = UP;

      replicas_[i].init();
    }

    for (size_t i = 0; i < N; i++)
    {
      this->evaluation_counter_ += replicas_[i].get_evaluation_counter();
    }

    if (N > 0)
    {
      this->update_lowest_cost(replicas_[0].cost(), replicas_[0].state());
    }
  }

  /// Decide whether to accept an exchange move
  double accept(const markov::Metropolis<Model_T>& a,
                const markov::Metropolis<Model_T>& b)
  {
    return this->rng_->uniform() <
           exp((a.cost() - b.cost()) * (a.beta() - b.beta()));
  }

  /// Perform discrete time step `t`.
  void make_step(uint64_t step) override
  {
    #pragma omp parallel for
    for (size_t i = 0; i < replicas_.size(); i++)
    {
      replicas_[i].make_sweeps(sweeps_per_replica_);
    }

    // Record observements
    for (size_t i = 0; i < replicas_.size(); i++)
    {
      auto label = this->scoped_observable_label("replica", i);
      auto& replica = replicas_[i];
      if (use_inverse_temperatures_)
      {
        this->observe("beta", replica.beta());
      }
      else
      {
        this->observe("T", replica.temperature());
      }
      auto counter = replica.get_evaluation_counter();
      this->evaluation_counter_ += counter;
      this->observe(
          "acc_rate",
          static_cast<double>(counter.get_accepted_transition_count()) /
              static_cast<double>(counter.get_difference_evaluation_count()));
      this->observe("avg_cost", replica.cost());

      this->update_lowest_cost(replica.get_lowest_cost(),
                               replica.get_lowest_state());

      replica.reset_evaluation_counter();
      this->observe("avg_dir", direction_[i]);
    }

    // Perform replica swaps
    for (size_t i = step & 1; i < replicas_.size() - 1; i += 2)
    {
      auto label = this->scoped_observable_label("replica", i);
      auto& a = replicas_[i];
      auto& b = replicas_[i + 1];
      if (accept(a, b))
      {
        a.swap_state(&b);
        std::swap(direction_[i], direction_[i + 1]);
        this->observe("swap_rate", 1);
      }
      else
      {
        this->observe("swap_rate", 0);
      }
    }
    direction_[0] = UP;
    direction_[direction_.size() - 1] = DOWN;
  }

  /// Configure
  void configure(const utils::Json& json) override
  {
    // Input parameters for the solver.
    Base_T::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kParams))
    {
      THROW(utils::MissingInputException, "Input field `", utils::kParams,
            "` is required.");
    }
    const utils::Json& params = json[utils::kParams];

    using ::matcher::GreaterThan;
    // sweeps has been used for setting up step_limit, so it could not be used
    // for setting sweeps_per_replica_
    this->param(params, "sweeps_per_replica", sweeps_per_replica_)
        .description("number of sweeps per replica between exchanges")
        .default_value((size_t)1)
        .matches(GreaterThan<size_t>(0));

    if (params.HasMember("temperatures"))
    {
      this->param(params, "temperatures", temperatures_)
          .description("temperatures for tempering")
          .required()
          .with_output();
      use_inverse_temperatures_ = false;
    }
    else if (params.HasMember("all_betas"))
    {
      this->param(params, "all_betas", temperatures_)
          .description("inverse temperatures for tempering")
          .required()
          .with_output();
      use_inverse_temperatures_ = true;
    }
    else
    {
      std::vector<double> default_betas = {0.1, 0.5, 1., 2., 4.};
      temperatures_.make_list(default_betas);
      use_inverse_temperatures_ = true;
      this->set_output_parameter("all_betas", temperatures_);
    }
  }

  void finalize() override { this->populate_solutions(replicas_); }

 protected:
  bool use_inverse_temperatures_;
  ::schedule::Schedule temperatures_;
  size_t sweeps_per_replica_;

 private:
  enum Direction
  {
    UP = 0,
    DOWN = 1
  };
  std::vector<::markov::Metropolis<Model_T>> replicas_;
  std::vector<std::unique_ptr<::utils::RandomGenerator>> rngs_;
  std::vector<Direction> direction_;
};
REGISTER_SOLVER(ParallelTempering);

}  // namespace solver
