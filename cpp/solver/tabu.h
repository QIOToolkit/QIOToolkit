// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <vector>

#include "utils/config.h"
#include "utils/exception.h"
#include "utils/operating_system.h"
#include "utils/random_generator.h"
#include "utils/random_selector.h"
#include "markov/tabu_walker.h"
#include "solver/stepping_solver.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
///
/// Defines a tabu search solver.
/// We are implementing the simple tabu search found in the following papers:
///  - J.E Beasley's 1998 paper "Heuristic algorithms for the unconstrained
///  binary
/// quadratic programming problem"
///  - GINTARAS PALUBECKIS 2004 paper "Multistart Tabu Search Strategies for the
/// Unconstrained Binary Quadratic Optimization Problem" with the following
/// adjustments:
/// - local delta calculations (as opposed to full re-calculation of the cost
/// function)
///- different starting configurations running in parallel
///

template <class Model_T>
class Tabu : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  Tabu() : restarts_(1) {}

  Tabu(const Tabu&) = delete;
  Tabu& operator=(const Tabu&) = delete;

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override { return "microsoft.tabu.qiotoolkit"; }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too many replicas, terms and/or "
        "variables)."
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
    #pragma omp parallel for
    for (size_t i = 0; i < restarts_; i++)
    {
      replicas_[i].set_model(this->model_);
      replicas_[i].set_rng(rngs_[i].get());
      replicas_[i].reset_evaluation_counter();
      replicas_[i].init();
    }
    for (size_t i = 0; i < restarts_; i++)
    {
      this->evaluation_counter_ += replicas_[i].get_evaluation_counter();
    }
    this->update_lowest_cost(replicas_[0].cost(), replicas_[0].state());
  }

  void make_step(uint64_t) override
  {
    #pragma omp parallel for
    for (size_t i = 0; i < restarts_; i++)
    {
      replicas_[i].reset_evaluation_counter();
      if (tabu_tenures_.size() > 0)
      {
        replicas_[i].set_tenure(tabu_tenures_[i]);
      }
      else
      {
        replicas_[i].set_tenure(tabu_tenure_);
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
    const utils::Json& params = json[utils::kParams];

    using ::matcher::GreaterEqual;

    this->param(params, "tabu_tenure", tabu_tenure_)
        .description(
            "tabu tenure, number of sweeps it is not allowed to apply same "
            "transition (change same variable).")
        .matches(GreaterEqual(static_cast<unsigned int>(0)))
        .default_value(static_cast<unsigned int>(0))
        .with_output();

    this->param(params, "restarts", restarts_)
        .description("number of restarts for tabu search solver")
        .default_value(static_cast<size_t>(this->thread_count_))
        .with_output()
        .matches(::matcher::GreaterThan<size_t>(0));
  }

  void finalize() override { this->populate_solutions(replicas_); }

 protected:
  size_t restarts_;
  std::vector<unsigned int> tabu_tenures_;
  unsigned int tabu_tenure_;

 private:
  std::vector<std::unique_ptr<::utils::RandomGenerator>> rngs_;
  std::vector<::markov::TabuWalker<Model_T>> replicas_;
};
REGISTER_SOLVER(Tabu);

}  // namespace solver
