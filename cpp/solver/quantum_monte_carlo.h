
#pragma once

#include <vector>

#include "utils/exception.h"
#include "utils/operating_system.h"
#include "utils/random_generator.h"
#include "utils/random_selector.h"
#include "markov/quantum_walker.h"
#include "schedule/schedule.h"
#include "solver/stepping_solver.h"

namespace solver
{
template <class Model_T>
class QuantumMonteCarlo : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  QuantumMonteCarlo()
      : restarts_(1), trotter_number_(this->thread_count_)
  {
    total_states_ = restarts_ * trotter_number_;
  }

  QuantumMonteCarlo(const QuantumMonteCarlo&) = delete;
  QuantumMonteCarlo& operator=(const QuantumMonteCarlo&) = delete;

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override
  {
    return "quantummontecarlo.qiotoolkit";
  }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too big trotter number, too many terms "
        "and/or variables). "
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override
  {
    return total_states_;
  }

  void init() override
  {
    this->init_memory_check();
    // proceed to memory allocation

    replicas_.resize(restarts_);

    size_t n_rngs = total_states_;
    rngs_.resize(n_rngs);
    for (size_t i = 0; i < n_rngs; i++)
    {
      rngs_[i] = this->rng_->fork();
    }

    init_replicas();

    for (size_t i = 0; i < restarts_; i++)
    {
      this->evaluation_counter_ += replicas_[i].get_evaluation_counter();
    }

    this->update_lowest_cost(replicas_[0].get_lowest_cost(),
                             replicas_[0].get_lowest_state());
  }

  void init_replicas()
  {
    #pragma omp parallel for
    for (size_t restart_index = 0; restart_index < restarts_; restart_index++)
    {
      auto& replica = replicas_[restart_index];
      replica.set_model(this->model_);
      replica.reset_evaluation_counter();
      replica.init(trotter_number_);
    }

    #pragma omp parallel for
    for (size_t i = 0; i < total_states_; i++)
    {
      size_t state_index = i % trotter_number_;
      size_t restart_index = i / trotter_number_;
      auto& replica = replicas_[restart_index];
      auto& rng = *rngs_[i].get();
      replica.init_state(state_index, rng);
    }

    #pragma omp parallel for
    for (size_t restart_index = 0; restart_index < restarts_; restart_index++)
    {
      auto& replica = replicas_[restart_index];
      replica.add_function_evaluations(trotter_number_);
      replica.save_lowest();
    }
  }

  void make_step(uint64_t step) override
  {
    double current_transverse_field =
        transverse_field_schedule_.get_value((double)step);

    double current_beta = beta_schedule_.get_value((double)step);

    double beta = calc_beta(current_beta);
    double bond_prob = calc_bond_prob(current_transverse_field, current_beta);

    make_sweeps(beta, bond_prob);

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


  void make_sweeps(double beta, double bond_prob)
  {
    size_t sweep_size = this->model_->get_sweep_size();
    std::vector<typename Model_T::Transition_T> transitions(restarts_);

    #pragma omp parallel
    {
      #pragma omp for
      for (size_t restart_index = 0; restart_index < restarts_; restart_index++)
      {
        auto& replica = replicas_[restart_index];
        replica.reset_evaluation_counter();
        replica.set_bond_probability(bond_prob);
        replica.set_beta(beta);
      }

      for (size_t variable_index = 0; variable_index < sweep_size;
           variable_index++)
      {
        #pragma omp for
        for (size_t restart_index = 0; restart_index < restarts_;
             restart_index++)
        {
          auto& replica = replicas_[restart_index];
          auto& rng = *rngs_[restart_index].get();
          replica.get_transition(variable_index, transitions[restart_index], rng);
        }

        // Functions inside for loops are using object private
        // members for passing results to use in the next loop.
        // Use caution during changing or merging loops.

        #pragma omp for
        for (size_t i = 0; i < total_states_; i++)
        {
          size_t state_index = i % trotter_number_;
          size_t restart_index = i / trotter_number_;

          auto& replica = replicas_[restart_index];
          auto& rng = *rngs_[i].get();
          replica.calculate_cost_diff(state_index, transitions[restart_index]);
          replica.try_form_clusters(state_index, transitions[restart_index], rng);
        }

        #pragma omp for
        for (size_t i = 0; i < total_states_; i++)
        {
          size_t state_index = i % trotter_number_;
          size_t restart_index = i / trotter_number_;

          auto& replica = replicas_[restart_index];
          auto& rng = *rngs_[i].get();
          replica.try_flip_clusters(state_index, rng);
        }

        #pragma omp for
        for (size_t i = 0; i < total_states_; i++)
        {
          size_t state_index = i % trotter_number_;
          size_t restart_index = i / trotter_number_;

          auto& replica = replicas_[restart_index];
          replica.apply_transition(state_index, transitions[restart_index]);
        }
      }

      #pragma omp for
      for (size_t restart_index = 0; restart_index < restarts_; restart_index++)
      {
        auto& replica = replicas_[restart_index];
        replica.add_difference_evaluations(trotter_number_ * sweep_size);
        replica.save_lowest();
      }
    }
  }

  utils::Structure get_solutions() const override
  {
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

    using ::matcher::GreaterEqual;
    using ::matcher::GreaterThan;
    using ::matcher::LessEqual;

    double beta_start, beta_stop;
    double transverse_field_low, transverse_field_high;

    this->param(params, "beta_stop", beta_stop)
        .description("Final inverse temperature")
        .matches(GreaterThan(0.0))
        .default_value(1.0)
        .with_output();

    // use dynamic beta only if 2 parameters are provided
    this->param(params, "beta_start", beta_start)
        .description("Starting inverse temperature")
        .matches(LessEqual(beta_stop))
        .default_value(beta_stop)
        .with_output();

    if ((!params.HasMember("beta_stop")) && params.HasMember("beta_start"))
    {
      beta_stop = beta_start;
    }

    beta_schedule_.make_linear(beta_start, beta_stop);
    beta_schedule_.set_stop(std::max(1, (int)this->step_limit_.value() - 1));

    this->param(params, "transverse_field_low", transverse_field_low)
        .description("Starting transverse field")
        .matches(GreaterEqual(0.0))
        .default_value(0.0)
        .with_output();

    this->param(params, "transverse_field_high", transverse_field_high)
        .description("Final transverse field")
        .matches(GreaterThan(transverse_field_low))
        .default_value(5.0)
        .with_output();

    transverse_field_schedule_.make_linear(transverse_field_low,
                                           transverse_field_high);
    transverse_field_schedule_.set_stop(
        std::max(1, (int)this->step_limit_.value() - 1));

    this->param(params, "trotter_number", trotter_number_)
        .description("Number of layers")
        .default_value(static_cast<size_t>(this->thread_count_))
        .matches(GreaterThan<size_t>(1))
        .with_output();

    this->param(params, "restarts", restarts_)
        .description("Number of restarts")
        .default_value(static_cast<size_t>(1))
        .with_output()
        .matches(::matcher::GreaterThan<size_t>(0));

    // To avoid potential memory overflow, limit number of restarts
    // to number of threads. Let to have 16 restarts to be able
    // to run same benchmark cases on local machines
    restarts_ = std::min(restarts_, (size_t)std::max(this->thread_count_, 16));
    total_states_ = restarts_ * trotter_number_;
  }

  // conversion from input parameters
  inline double calc_beta(double beta) { return beta / trotter_number_; }

  inline double calc_bond_prob(double transverse_field, double beta)
  {
    return 1 - std::tanh(transverse_field * beta / trotter_number_);
  }

  void finalize() override { this->populate_solutions(replicas_); }

 protected:
  ::schedule::Schedule beta_schedule_;
  ::schedule::Schedule transverse_field_schedule_;
  size_t restarts_;
  size_t trotter_number_;
  size_t total_states_;

  std::vector<std::unique_ptr<::utils::RandomGenerator>> rngs_;
  std::vector<::markov::QuantumWalker<Model_T>> replicas_;
};
REGISTER_SOLVER(QuantumMonteCarlo);

}  // namespace solver
