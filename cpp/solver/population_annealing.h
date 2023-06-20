// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <vector>

#include "utils/config.h"
#include "utils/exception.h"
#include "utils/operating_system.h"
#include "utils/random_generator.h"
#include "utils/random_selector.h"
#include "markov/metropolis.h"
#include "schedule/schedule.h"
#include "solver/population.h"
#include "solver/stepping_solver.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// Population Annealing
///
/// Population based optimizer with variable beta stepping. PopulationAnnealing
/// explores the configuration space using multiple Metropolis walkers subject
/// to a birth-death process (depending on their relative cost-fitness within
/// the population).
///
/// Stepping method
///
/// Population annealing supports three different `resampling_strategy`s for
/// dynamically selecting the next step in temperature space (`delta_beta`):
///
/// * `friction_tensor`: The stepping size is calculated from
///
///   \f[ \Delta\beta = c_{FT} / \sqrt\zeta \f]
///
///   ```json
///   {
///     'target': 'microsoft.populationannealing.cpu',
///     'version': '1.0',
///     'resampling_strategy': 'friction_tensor',
///     'friction_tensor_constant': 1
///   }
///   ```
///
/// * `energy_variance`: Scale temperature stepping with the variance in
///   the energy (cost):
///
///   \f[ \Delta\beta = f_{culling} / \sqrt\sigma \f]
///
///   for this, the _initial_ value for the culling fraction needs to be
///   specified:
///
///   ```json
///   {
///     'target': 'microsoft.populationannealing.cpu',
///     'version': '1.0',
///     'resampling_strategy': 'energy_variance',
///     'initial_culling_fraction': 0.5
///   }
///   ```
///
/// * `constant_culling`: The stepping size is chosen to keep the culling
///   rate constant at `culling_fraction` (by estimating the expected death
///   ratio and solving for `delta_beta`):
///
///   ```json
///   {
///     'target': 'microsoft.populationannealing.cpu',
///     'version': '1.0',
///     'resampling_strategy': 'constant_culling',
///     'culling_fraction': 0.1
///   }
///   ```
///
template <class Model_T>
class PopulationAnnealing : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  PopulationAnnealing() : target_population_(0), epoch_(0) {}

  PopulationAnnealing(const PopulationAnnealing&) = delete;
  PopulationAnnealing& operator=(const PopulationAnnealing&) = delete;

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override
  {
    return "microsoft.populationannealing.cpu";
  }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too many replicas, terms and/or "
        "variables)."
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override { return target_population_; }

  void init() override
  {
    // check memory needed for method
    this->init_memory_check();
    // proceed to memory allocation

    // Initialize the rngs
    rngs_.resize(static_cast<size_t>(omp_get_max_threads()));
    for (size_t t = 0; t < rngs_.size(); t++)
    {
      rngs_[t] = this->rng_->fork();
    }

    // Rescale the bonds in the model
    if (!this->model_->is_rescaled())
    {
      const_cast<Model_T*>(this->model_)->rescale();
    }

    // Create the population
    if (population_.empty())
    {
      if (target_population_ == 0)
      {
        throw(utils::PopulationIsEmptyException(
            "target_population must be greater than 0"));
      }
      cur_beta_ = beta_start_;
      restart_base_step_ = 0;
      init_population();
      this->update_lowest_cost(population_[0]->cost(), population_[0]->state());
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /// Reset the population with size `target_population`
  ///
  /// NOTE: This method is invoked both at the start of the simulation and
  /// whenever a restart/population increase is triggered.
  void init_population()
  {
    population_.reserve(target_population_);
    ::markov::Metropolis<Model_T> blueprint;
    blueprint.set_beta(cur_beta_);
    blueprint.set_model(this->model_);
    blueprint.set_rng(this->rng_.get());

    for (size_t i = 0; i < target_population_; i++)
    {
      if (population_.size() <= i)
      {
        population_.insert(blueprint);
      }
      population_[i].set_family(i);
    }

    #pragma omp parallel for schedule(static, 1)
    for (size_t i = 0; i < population_.size(); i++)
    {
      size_t thread_id = static_cast<size_t>(omp_get_thread_num());
      population_[i]->set_rng(rngs_[thread_id].get());
      population_[i]->init();
    }

    current_culling_fraction_ = initial_culling_fraction_;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// The CullingEstimator is used to infer the expected culling ratio
  /// for a proposed temperature stepping delta_beta for the
  /// "constant_culling" stepping method.
  ///
  /// (It is used by the find_delta_beta method below to iteratively find the
  /// appropriate stepping to keep culling approximately constant).
  class CullingEstimator
  {
   public:
    CullingEstimator(const std::vector<double>& costs, double epsilon0,
                     size_t target_population, double scale_factor)
        : costs_(costs),
          epsilon0_(epsilon0),
          target_population_(target_population),
          scale_factor_(scale_factor)
    {
    }

    CullingEstimator(const CullingEstimator&) = delete;
    CullingEstimator& operator=(const CullingEstimator&) = delete;
    CullingEstimator& operator=(CullingEstimator&&) = delete;

    double operator()(double delta_beta)
    {
      size_t R = costs_.size();
      std::vector<double> weight(R);
      double Q = 0;

      for (size_t i = 0; i < R; i++)
      {
        assert(costs_[i] >= 0);  // We assume these were shifted!
        weight[i] = exp(-delta_beta * costs_[i] * scale_factor_);
        Q += weight[i];
        assert(std::isfinite(Q));
      }
      double RRQ = static_cast<double>(target_population_) / Q;
      double result = 0;
      for (size_t i = 0; i < R; i++)
      {
        double value = 1 - weight[i] * RRQ;
        if (value > 0) result += value;
      }
      return result / static_cast<double>(R) - epsilon0_;
    }

   private:
    const std::vector<double>& costs_;
    double epsilon0_;
    size_t target_population_;
    double scale_factor_;
  };

  /////////////////////////////////////////////////////////////////////////////////////////
  /// Adjust the stepping size delta_beta until the estimator returns
  /// an estimate of zero.
  /// NOTE: This is NOT a generic root finder, it makes several assumptions
  /// about the estimator being used.
  template <class T>
  double find_delta_beta(T& estimator)
  {
    double lower = 0, upper = 0;
    double lower_value = estimator(0);
    double upper_value;
    int e;
    // precision 10e-6, max step size 10e3
    for (e = -20; e < 20; e++)
    {
      upper = pow(2, e);
      upper_value = estimator(upper);
      if ((lower_value < 0) == (upper_value < 0))
      {
        lower = upper;
        lower_value = upper_value;
      }
      else
      {
        break;
      }
    }
    for (e -= 2; e >= -20; e--)
    {
      upper = lower + pow(2, e);
      upper_value = estimator(upper);
      if ((lower_value < 0) == (upper_value < 0))
      {
        lower = upper;
        lower_value = upper_value;
      }
    }
    return lower;
  }

  void make_step(uint64_t step) override
  {
    this->observe("epoch", static_cast<double>(epoch_));

    if (population_.empty())
    {
      throw(utils::PopulationIsEmptyException("population is empty."));
    }

    utils::enable_overflow_divbyzero_exceptions();

    // Record cost before sweeps for observables and stepping estimator.
    size_t R = population_.size();
    double Rd = static_cast<double>(R);
    costs_before_.resize(R);
    costs_after_.resize(R);

    // Perform `input_params.sweeps` Metropolis sweeps per citizen.
    #pragma omp parallel for schedule(static, 1)
    for (size_t i = 0; i < R; i++)
    {
      costs_before_[i] = population_[i]->cost();
      size_t thread_id = static_cast<size_t>(omp_get_thread_num());
      auto& citizen = population_[i];
      citizen->set_rng(rngs_[thread_id].get());

      // each sweep may take quite some time, and the R may be large for
      // population annealing so we need to double check the time_limit_ to
      // quit faster.
      if ((!this->time_limit_.has_value()) ||
          (utils::get_wall_time() - this->start_time_) <
              this->time_limit_.value())
      {
        citizen->make_sweeps(sweeps_per_replica_);
      }

      costs_after_[i] = citizen->cost();
    }

    for (size_t i = 0; i < population_.size(); i++)
    {
      auto& citizen = population_[i];
      this->update_lowest_cost(citizen->get_lowest_cost(),
                               citizen->get_lowest_state());
      this->evaluation_counter_ += citizen->get_evaluation_counter();
      citizen->reset_evaluation_counter();
    }

    // Observables for stepping size
    double avg_before = 0;
    double avg_after = 0;
    double min_cost = std::numeric_limits<double>::max();

#ifdef _MSC_VER
    #pragma omp parallel
    {
      // reduction min or max is not implemented yet in all compilers
      double min_cost_local = std::numeric_limits<double>::max();
      #pragma omp for reduction(+ : avg_before, avg_after)
      for (size_t i = 0; i < R; i++)
      {
        avg_before += costs_before_[i];
        avg_after += costs_after_[i];
        min_cost_local =
            costs_after_[i] < min_cost_local ? costs_after_[i] : min_cost_local;
      }
      #pragma omp critical
      min_cost = min_cost_local < min_cost ? min_cost_local : min_cost;
    }
#else
    #pragma omp parallel for reduction(+ : avg_before, avg_after) reduction(min : min_cost)
    for (size_t i = 0; i < R; i++)
    {
      avg_before += costs_before_[i];
      avg_after += costs_after_[i];
      min_cost = (costs_after_[i] < min_cost) ? costs_after_[i] : min_cost;
    }
#endif
    avg_before /= Rd;
    avg_after /= Rd;
    this->observe("cost", avg_after);
    this->observe("population", Rd);

    double var_before = 0;
    double var_after = 0;
    double gamma = 0;
    #pragma omp parallel for reduction(+ : var_before, var_after, gamma)
    for (size_t i = 0; i < population_.size(); i++)
    {
      double diff_before = costs_before_[i] - avg_before;
      double diff_after = costs_after_[i] - avg_after;
      var_before += diff_before * diff_before;
      var_after += diff_after * diff_after;
      gamma += diff_before * diff_after;
    }
    var_before /= Rd;
    var_after /= Rd;
    gamma /= Rd;
    double sigma = sqrt(var_before * var_after);
    this->observe("sigma", sigma);
    double zeta = 0;
    if (sigma > 0 && gamma != 0)
    {
      double delta = log(sigma) - log(std::abs(gamma));
      if (std::abs(delta) > 10E-12)
      {
        // avoid divided by 0 and floating point calculation error
        double tau = static_cast<double>(sweeps_per_replica_) / delta;
        zeta = sigma * tau;
        this->observe("tau", tau);
        this->observe("zeta", zeta);
      }
    }

    // We shift the costs after to be greater-equal 0 (for numerical stability
    // during resampling; see description in resample).
    #pragma omp parallel for
    for (size_t i = 0; i < population_.size(); i++)
    {
      costs_after_[i] -= min_cost;
    }

    // Select stepping size
    double delta_beta = 0.0;
    if (is_linear_schedule() || is_geometric_schedule())
    {
      assert(restart_base_step_ <= step);
      delta_beta = beta_.get_value(step + 1 - restart_base_step_) -
                   beta_.get_value(step - restart_base_step_);
    }
    else if (is_friction_tensor())
    {
      // Friction tensor method: \Delta\beta \propto \frac{1}/{\sqrt{\zeta}}
      if (zeta > 0)
      {
        delta_beta = friction_tensor_constant_ / sqrt(zeta);
      }
    }
    else if (is_energy_variance())
    {
      // Energy variance method: \Delta\beta \propto
      //   \frac{\epsilon}{\sqrt{\sigma}}
      if (sigma > 0)
      {
        delta_beta = current_culling_fraction_ / sqrt(sigma);
        LOG(INFO, "delta_beta: ", delta_beta, " current_culling_fraction_",
            current_culling_fraction_);
      }
    }
    else if (is_constant_culling())
    {
      // Fixed culling fraction method: Find \beta such that
      //   \epsilon(\beta) = \epsilon_0, in which
      //   \epsilon(\Delta B) = \frac1R\sum_{n=1}^R
      //                        (1-\frac{e^{-\Delta\beta E_n}{Q})
      //                        \StepFunction(1-e^{-\Delta\betaE_n}{Q})
      CullingEstimator estimator(costs_after_, constant_culling_fraction_,
                                 target_population_,
                                 this->model_->get_scale_factor());
      delta_beta = find_delta_beta(estimator);
      LOG(INFO, "delta_beta: ", delta_beta);
    }

    double rho_t = 1.0;
    if (delta_beta > 0)
    {
      // Resampling according to selected delta_beta
      this->observe("delta_beta", delta_beta);
      resample(delta_beta);

      // Family observables
      auto& families = population_.get_families();
      double family_count = static_cast<double>(families.size());
      this->observe("families", family_count);
      rho_t = 0;
      for (const auto& id_and_size : families)
      {
        double family_size = static_cast<double>(id_and_size.second);
        rho_t += family_size * family_size;
        this->observe("family_size", family_size, 1.);
      }
      rho_t /= Rd;
      this->observe("rho_t", rho_t);
    }

    // This condition checks for two things:
    //
    // - if the lhs is less than alpha_, the population consists of "few"
    //   families (meaning of "few" is indicated by alpha). This means that
    //   all citizens are now descendants of one (or "few") ancestors.
    //
    // - The variance in the cost being zero, meaning all citizens have
    //   collapsed into a common cost state.
    //
    // Both of these are signals that further (local) search with this
    // population is inefficient and we are better suited with a "restart",
    // i.e., we initialize the population with new (random) initial states
    // and start at the initial beta.
    //
    if (Rd / rho_t < alpha_ || var_after == 0)
    {
      cur_beta_ = beta_start_;
      restart_base_step_ = step;
      if (utils::feature_enabled(utils::FEATURE_PA_EXP_REPOPULATION))
      {
        // only grow the population at the restart if this feature is enabled
        target_population_ *= 2;
        LOG(INFO, "restarting; increased population -> ", target_population_);
      }
      // restart the population
      init_population();
      observe::Observer::restart(step);
      epoch_++;
    }
    else
    {
      cur_beta_ += delta_beta;
      #pragma omp parallel for
      for (size_t i = 0; i < population_.size(); i++)
      {
        auto& citizen = population_[i];
        citizen->set_beta(cur_beta_);
      }
    }

    utils::disable_overflow_divbyzero_exceptions();
  }

  void resample(double delta_beta)
  {
    if (population_.size() == 0)
    {
      LOG(WARN, "resampling empty population");
      return;
    }

    // This loop assigns weights proportial to the Boltzmann distribution for
    // resampling and sums up the total for later renormalization
    // (i.e., the partition function in physics terms).
    //
    //   weight_i = exp(-delta_beta * cost_i) * C
    //   total_sum = C * \sum_i weight_i
    //   relative_weight_i = weight_i / total_sum
    //
    // Since the weights need only be proportional, we are free to multiply them
    // by the same constant C. We can use this to actively avoid overflow (both
    // in the individual exp() calls and the final sum) by choosing
    //
    //   C = exp(delta_beta * min_cost_in_pop)
    //
    // This gives the following weight formula:
    //
    //   weight_i = exp(delta_beta (min_cost_in_pop - cost_i)),
    //
    // Note that the the inner parenthesis is less-equal 0 and each individual
    // weight is therefore less-equal 1 (and at least one is guaranteed to be
    // 1, making total_weight > 0).
    //
    double total_weight = 0.0;
    std::vector<double> weight(population_.size(), 0.0);
    double scale_factor = this->model_->get_scale_factor();
    if (scale_factor <= 0)
    {
      THROW(utils::InconsistentValueException,
            "scale factor must be >=0, found ", scale_factor);
    }

    #pragma omp parallel for reduction(+ : total_weight)
    for (size_t i = 0; i < population_.size(); i++)
    {
      // Compute the relative weight τj (βk, βk−1) {Eq. (1)}
      // NOTE: costs_after_[i] has already been shifted to be >=0 at this stage.
      assert(costs_after_[i] >= 0);
      weight[i] = exp(-delta_beta * costs_after_[i] * scale_factor);
      assert(std::isfinite(weight[i]));
      total_weight += weight[i];
      assert(std::isfinite(total_weight));
    }

    if (constant_population_)
    {
      // Method forcing the population to be constant
      size_t resampled_size = 0;
      std::vector<double> residual_weights(population_.size(), 0);
      #pragma omp parallel for reduction(+ : resampled_size)
      for (size_t i = 0; i < population_.size(); i++)
      {
        double relative_weight =
            static_cast<double>(target_population_) * weight[i] / total_weight;
        size_t int_weight = (size_t)floor(relative_weight);
        double residual_weight =
            relative_weight - static_cast<double>(int_weight);
        population_[i].set_count(int_weight);
        residual_weights[i] = residual_weight;
        resampled_size += int_weight;
      }
      // We use a RandomSelector to fill the population to the desired size.
      // (this helper class ensures the filling citizens are chosen with
      // probability proportional to their residual weight). As a result,
      // each citizen is selected
      //
      //   * at least `floor(relative_weight)` times
      //   * at most `ceil(relative_weight)` times
      //   * has a chance to be selected an additional time of
      //     `relative_weight - floor(relative_weight)`
      //     (i.e., the fractional part of `relative_weight`)
      ::utils::RandomSelector selector;
      for (size_t i = 0; i < population_.size(); i++)
      {
        selector.insert(i, residual_weights[i]);
      }
      while (resampled_size < population_.size())
      {
        size_t citizen_id = selector.select_and_remove(this->rng_->uniform());
        population_[citizen_id].spawn(1);
        resampled_size++;
      }
    }
    else
    {
      // Original method (variable population within +-5% of target)
      // Compute the partition function ratio Q(βk, βk−1) * population_.size();
      double Q = total_weight;
      uint64_t current_population = population_.size();
      double RRQ = static_cast<double>(target_population_) / Q;
      for (size_t i = 0; i < population_.size(); i++)
      {
        auto& citizen = population_[i];
        // Resample: make N[(Rβk−1 /R˜ βk)τj (βk, βk−1)] copies of replica j
        // {N(a) is a Poisson random variate with mean a}
        // taking into account Poisson is not defined at 0.
        size_t new_count =
            (RRQ * weight[i]) > 0.0
                ? (size_t)floor(this->rng_->poisson(RRQ * weight[i]))
                : size_t(0);
        if (new_count == 0)
        {
          current_population -= citizen.get_count();
          citizen.kill();
        }
        else if (new_count > 1)
        {
          current_population += new_count - 1;
          citizen.spawn(new_count - 1);
        }
      }
      // Keep population alive feature
      if (current_population == 0)
      {
        size_t i = (size_t)floor(this->rng_->uniform() * population_.size());
        population_[i].spawn(1);
      }
    }
    population_.resample();
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

    using ::matcher::GreaterEqual;
    using ::matcher::GreaterThan;

    this->param(params, "alpha", alpha_)
        .description("ratio to trigger a restart")
        .default_value(2.0)
        .matches(GreaterThan(1.0))
        .with_output();

    this->param(params, "population", target_population_)
        .description("population size")
        .default_value((size_t)this->thread_count_)
        .matches(GreaterThan<size_t>(0))
        .with_output();

    this->param(params, "constant_population", constant_population_)
        .description("whether to keep the population constant between restarts")
        .default_value(false)
        .with_output();

    this->param(params, "resampling_strategy", resampling_strategy_)
        .description(
            "resample by 'linear_schedule', 'geometric_schedule, "
            "'friction_tensor', 'energy_variance' or 'constant_culling'.")
        .default_value((std::string) "linear_schedule");

    initial_culling_fraction_ = 0.5;

    if (is_linear_schedule() || is_geometric_schedule())
    {
      // This is the current default resampling schedule (i.e., according
      // to a linear schedule in beta).
      bool using_default_beta = false;
      if (params.HasMember("beta"))
      {
        this->param(params, "beta", beta_)
            .description("how to evolve beta (resampling factor) over time.");
        beta_start_ = beta_.get_initial();
        beta_stop_ = beta_.get_final();
      }
      else
      {
        // For compatibility with microsoft.simulatedannealing.cpu, allow the
        // annealing schedule to be specified with beta_low, beta_high.
        this->param(params, "beta_start", beta_start_)
            .description("the initial inverse temperature for the schedule")
            .default_value(0.0)
            .with_output();
        this->param(params, "beta_stop", beta_stop_)
            .description("the final inverse temperature for the schedule.")
            .default_value(5.0)
            .matches(GreaterEqual(beta_start_))
            .with_output();
        using_default_beta = true;

        if (is_linear_schedule())
        {
          this->param(params, "beta_start", beta_start_)
              .matches(GreaterEqual(0.0));

          beta_.make_linear(beta_start_, beta_stop_);
        }
        else if (is_geometric_schedule())
        {
          // Start and stop must be positive for geometric
          this->param(params, "beta_start", beta_start_)
              .default_value(0.01)
              .matches(GreaterThan(0.0));
          this->param(params, "beta_stop", beta_stop_)
              .matches(GreaterThan(0.0));

          beta_.make_geometric(beta_start_, beta_stop_);
        }
      }
      // If the beta schedule does not inherently have a range (start & stop) or
      // count (intended number of steps), stretch the schedule to the number
      // of steps given by step_limit.
      //
      // NOTE: The effect of this is that population annealing will do a single
      // descent over the number of steps the simulation will run.
      if (beta_.get_count() == 1)
      {
        if (this->step_limit_.has_value() && this->step_limit_.value() > 1)
        {
          if (!using_default_beta)
            LOG(INFO, "Stretching population_annealing.beta to ",
                this->step_limit_.value(), " steps. You can change this by ",
                "explicitly setting schedule.{start,stop} or schedule.count.");
          beta_.set_stop(std::max(1, (int)this->step_limit_.value() - 1));
        }
        else
        {
          throw utils::MissingInputException(
              "Either a step_limit or schedule.count must be provided. "
              "You can also set beta.{start,stop} to define the range.");
        }
      }
      this->set_output_parameter("beta", beta_);
      if (beta_.get_initial() >= beta_.get_final())
      {
        THROW(utils::ValueException, "the schedule for beta must be increasing ",
              "(i.e., beta.initial < beta.final). ",
              "Found beta.initial = ", beta_.get_initial(),
              " >= ", beta_.get_final(), " = beta.final.");
      }
      beta_start_ = beta_.get_value(0);
    }
    else
    {
      // These are experimental resampling strategies
      // (not advertised in the service).
      beta_start_ = 0;
      if (is_friction_tensor())
      {
        this->param(params, "friction_tensor_constant",
                    friction_tensor_constant_)
            .default_value(1.0)
            .matches(GreaterThan(0.0))
            .with_output();
      }
      else if (is_energy_variance())
      {
        this->param(params, "initial_culling_fraction",
                    initial_culling_fraction_)
            .default_value(0.5)
            .matches(GreaterThan(0.0))
            .with_output();
      }
      else if (is_constant_culling())
      {
        this->param(params, "culling_fraction", constant_culling_fraction_)
            .description("constant culling rate")
            .default_value(0.1)
            .matches(GreaterThan(0.0))
            .with_output();
      }
      else
      {
        throw utils::InitialConfigException(
            "`resampling strategy` must be one of ['linear_schedule', "
            " 'geometric_schedule', 'friction_tensor', 'energy_variance', "
            "'constant_culling'].");
      }
    }

    this->param(params, "sweeps_per_replica", sweeps_per_replica_)
        .description("sweeps per replica between resampling.")
        .default_value((size_t)1)
        .matches(GreaterThan<size_t>(0))
        .with_output();
  }

  void finalize() override
  {
    // adjust population and current lowest solution with scale factor before
    // persisting.
    this->populate_solutions(population_);
    this->population_.clear_cache_pool();
  }

 protected:
  /////////////////////////////////////////////////////////////////////////////
  /// expand the population to size `target_population`
  ///
  /// NOTE: This method is called to expand population from current size to
  /// target_population
  void expand_population()
  {
    cur_beta_ = beta_start_;
    restart_base_step_ = 0;

    population_.reserve(target_population_);
    ::markov::Metropolis<Model_T> blueprint;
    blueprint.set_beta(cur_beta_);
    blueprint.set_model(this->model_);
    blueprint.set_rng(this->rng_.get());
    const size_t index_start = population_.size();
    for (size_t i = 0; i < target_population_; i++)
    {
      if (i >= index_start)
      {
        population_.insert(blueprint);
      }
      population_[i].set_family(i);
    }

    #pragma omp parallel for schedule(static, 1)
    for (size_t i = 0; i < population_.size(); i++)
    {
      if (i >= index_start)
      {
        size_t thread_id = static_cast<size_t>(omp_get_thread_num());
        population_[i]->set_rng(rngs_[thread_id].get());
        population_[i]->init();
      }
    }

    current_culling_fraction_ = initial_culling_fraction_;
    this->update_lowest_cost(population_[0]->cost(), population_[0]->state());
  }

  bool is_linear_schedule() const
  {
    return this->resampling_strategy_ == "linear_schedule";
  }

  bool is_geometric_schedule() const
  {
    return this->resampling_strategy_ == "geometric_schedule";
  }

  bool is_friction_tensor() const
  {
    return this->resampling_strategy_ == "friction_tensor";
  }

  bool is_energy_variance() const
  {
    return this->resampling_strategy_ == "energy_variance";
  }

  bool is_constant_culling() const
  {
    return this->resampling_strategy_ == "constant_culling";
  }
  double alpha_;
  double beta_start_, beta_stop_;
  size_t target_population_;
  size_t sweeps_per_replica_;
  bool constant_population_;
  std::string resampling_strategy_;

  ::schedule::Schedule beta_;
  Population<::markov::Metropolis<Model_T>> population_;
  double friction_tensor_constant_;
  double initial_culling_fraction_;
  double constant_culling_fraction_;

 private:
  std::vector<double> costs_before_;
  std::vector<double> costs_after_;
  double current_culling_fraction_;

  std::vector<std::unique_ptr<::utils::RandomGenerator>> rngs_;

  double cur_beta_;
  size_t epoch_;

  uint64_t restart_base_step_;
};
REGISTER_SOLVER(PopulationAnnealing);

}  // namespace solver
