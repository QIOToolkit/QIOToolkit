
#pragma once

#include <vector>

#include "utils/exception.h"
#include "utils/operating_system.h"
#include "utils/random_generator.h"
#include "utils/random_selector.h"
#include "markov/random_walker.h"
#include "schedule/schedule.h"
#include "solver/population.h"
#include "solver/stepping_solver.h"

namespace solver
{
template <class Model_T>
class SubstochasticMonteCarlo : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  SubstochasticMonteCarlo() : target_population_(0) {}

  SubstochasticMonteCarlo(const SubstochasticMonteCarlo&) = delete;
  SubstochasticMonteCarlo& operator=(const SubstochasticMonteCarlo&) = delete;

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override
  {
    return "substochasticmontecarlo.cpu";
  }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too big target population, too many terms "
        "and/or variables). "
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override { return target_population_; }

  void init() override
  {
    if (population_.empty())
    {
      if (target_population_ == 0)
      {
        throw(utils::PopulationIsEmptyException(
            "target_population must be greater than 0"));
      }
      // check memory needed for method
      this->init_memory_check();
      rngs_.resize(static_cast<size_t>(omp_get_max_threads()));
      for (size_t t = 0; t < rngs_.size(); t++)
      {
        rngs_[t] = this->rng_->fork();
      }
      // proceed to memory allocation

      // Prepare the population
      prepare_population();
    }
  }

  void make_step(uint64_t step) override
  {
    if (population_.empty())
    {
      throw(utils::PopulationIsEmptyException("population is empty."));
    }

    utils::enable_overflow_divbyzero_exceptions();
    // perform random walk with the current (step-dependent)
    // random stepping probability `alpha`.
    make_walker_steps(alpha_.get_value((double)step));
    // Update the population statistics (needed for resampling, but
    // also for tracking & dumping metadata).
    update_population_statistics();
    // Apply resampling with (step-dependent) weighting `beta`.
    resample_population(beta_.get_value((double)step));
    utils::disable_overflow_divbyzero_exceptions();
  }

  void make_walker_steps(double alpha)
  {
    double p_step = alpha / static_cast<double>(steps_per_walker_);

    // since there are a lot of execution time randomness because of
    // rngs_[thread_id]->uniform() < p_step we have to use dynamic scheduling
    // here this will incur repro issue, which we will resolve later by assign
    // each citizen a RNG.
    //
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < population_.size(); i++)
    {
      size_t thread_id = static_cast<size_t>(omp_get_thread_num());
      population_[i]->set_rng(rngs_[thread_id].get());
      for (size_t k = 0; k < steps_per_walker_; k++)
      {
        if (rngs_[thread_id]->uniform() < p_step)
        {
          // This is deliberately a (non-sytem-size-dependent) single step,
          // rather than a sweep as in several other solvers.
          population_[i]->make_step();
        }
      }
    }
  }

  void update_population_statistics()
  {
    max_cost_ = min_cost_ = population_[0]->cost();
    for (size_t i = 0; i < population_.size(); i++)
    {
      const auto& citizen = population_[i];
      min_cost_ = std::min(min_cost_, double(citizen->cost()));
      max_cost_ = std::max(max_cost_, double(citizen->cost()));
      this->update_lowest_cost(citizen->get_lowest_cost(),
                               citizen->get_lowest_state());
    }
  }

  void resample_population(double beta)
  {
    assert(max_cost_ >= min_cost_);
    double cost_range = max_cost_ - min_cost_;
    if (cost_range > 0)
    {
      double total_weight = 0;
      std::vector<double> weight(population_.size());
      for (size_t i = 0; i < population_.size(); i++)
      {
        const auto& citizen = population_[i];
        weight[i] = exp(beta * (max_cost_ - citizen->cost()) / cost_range);
        total_weight += weight[i];
      }

      size_t resampled_size = 0;
      ::utils::RandomSelector selector;
      for (size_t i = 0; i < population_.size(); i++)
      {
        auto& citizen = population_[i];
        double relative_weight =
            static_cast<double>(target_population_) * weight[i] / total_weight;
        size_t int_weight = (size_t)floor(relative_weight);
        double residual_weight =
            relative_weight - static_cast<double>(int_weight);
        citizen.set_count(int_weight);
        resampled_size += int_weight;
        selector.insert(i, residual_weight);
      }
      assert(resampled_size <= target_population_);
      while (resampled_size < population_.size())
      {
        size_t citizen_id = selector.select_and_remove(this->rng_->uniform());
        population_[citizen_id].spawn(1);
        resampled_size++;
      }
      population_.resample();
    }
  }

  utils::Structure get_solutions() const override
  {
#ifdef _DEBUG
    double rec_cost = min_cost_;
    std::cerr << "recorded cost:" << std::fixed << rec_cost << std::endl;
#endif
    return Base_T::get_solutions();
  }

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

    this->param(params, "target_population", target_population_)
        .description("desired population size")
        .default_value((size_t)this->thread_count_)
        .matches(GreaterThan<size_t>(0))
        .with_output();

    this->param(params, "steps_per_walker", steps_per_walker_)
        .description("Number of steps to attempt for each walker.")
        .default_value((size_t)1)
        .matches(GreaterThan<size_t>(0))
        .with_output();

    alpha_.make_linear(1.0, 0.0);
    this->param(params, "alpha", alpha_)
        .description("how to evolve alpha (chance to step) over time.");

    beta_.make_linear(0.0, 5.0);
    this->param(params, "beta", beta_)
        .description("how to evolve beta (resampling factor) over time.");

    // Ensure alpha_ and beta_ have a length and they are equal.
    if (alpha_.get_count() <= 1 && beta_.get_count() <= 1)
    {
      // Neither has a length, try using the step_limit_ as length.
      if (this->step_limit_.has_value())
      {
        alpha_.set_stop(this->step_limit_.value());
        beta_.set_stop(this->step_limit_.value());
      }
      else
      {
        throw utils::MissingInputException(
            "You must set either the step_limit or the range for alpha and "
            "beta");
      }
    }
    else if (alpha_.get_count() <= 1)
    {
      // only beta_ has a length, copy it to alpha_
      alpha_.set_stop(beta_.get_stop());
    }
    else if (beta_.get_count() <= 1)
    {
      // only alpha_ has a length, copy it to beta_
      beta_.set_stop(alpha_.get_stop());
    }
    else if (alpha_.get_count() != beta_.get_count())
    {
      // both have a length but they don't match.
      throw utils::ValueException(
          "The schedules for alpha and beta must have the same length");
    }

    // Ensure that alpha is decreasing
    if (alpha_.get_initial() <= alpha_.get_final())
    {
      THROW(utils::ValueException, "the schedule for alpha must be decreasing ",
            "(i.e., alpha.initial > alpha.final). ",
            "Found alpha.initial = ", alpha_.get_initial(),
            " <= ", alpha_.get_final(), " = alpha.final.");
    }

    // Ensure that beta is increasing
    if (beta_.get_initial() >= beta_.get_final())
    {
      THROW(utils::ValueException, "the schedule for beta must be increasing ",
            "(i.e., beta.initial < beta.final). ",
            "Found beta.initial = ", beta_.get_initial(),
            " >= ", beta_.get_final(), " = beta.final.");
    }

    this->set_output_parameter("alpha", alpha_);
    this->set_output_parameter("beta", beta_);
  }

  void finalize() override
  {
    this->populate_solutions(population_);
    this->population_.clear_cache_pool();
  }

 private:
  std::vector<std::unique_ptr<::utils::RandomGenerator>> rngs_;

 protected:
  /////////////////////////////////////////////////////////////////////////////
  /// prepare the population to size `target_population`
  ///
  /// NOTE: This method is called to prepare population from current size to
  /// target_population
  void prepare_population()
  {
    ::markov::RandomWalker<Model_T> blueprint;
    blueprint.set_model(this->model_);
    blueprint.set_rng(this->rng_.get());
    this->population_.reserve(target_population_);
    const size_t start_indx = this->population_.size();
    for (size_t i = start_indx; i < target_population_; i++)
    {
      this->population_.insert(blueprint);
    }
    #pragma omp parallel for
    for (size_t i = 0; i < this->population_.size(); i++)
    {
      if (i >= start_indx)
      {
        size_t thread_id = static_cast<size_t>(omp_get_thread_num());
        this->population_[i]->set_rng(this->rngs_[thread_id].get());
        this->population_[i]->init();
      }
    }
    this->update_lowest_cost(this->population_[0]->cost(),
                             this->population_[0]->state());
  }
  size_t target_population_;
  size_t steps_per_walker_;
  ::schedule::Schedule alpha_;
  ::schedule::Schedule beta_;
  Population<::markov::RandomWalker<Model_T>> population_;
  double min_cost_, max_cost_;
};
REGISTER_SOLVER(SubstochasticMonteCarlo);

}  // namespace solver
