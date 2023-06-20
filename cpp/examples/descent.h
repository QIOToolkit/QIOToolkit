// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include "utils/random_generator.h"
#include "solver/stepping_solver.h"

namespace examples
{
template <class Model_T>
class Descent : public ::solver::SteppingSolver<Model_T>
{
 public:
  using Base_T = ::solver::SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;
  using Transition_T = typename Model_T::Transition_T;

  std::string get_identifier() const override
  {
    return "microsoft.descent.qiotoolkit";
  }

  void configure(const utils::Json& json) override
  {
    Base_T::configure(json);
    this->param(json[utils::kParams], "samples", samples_)
        .description("number of samples to take for the gradient")
        .default_value(10)
        .matches(matcher::GreaterThan(0))
        .with_output();
  }

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large."
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override { return 1; }

  void init() override
  {
    this->init_memory_check();
    state_ = this->model_->get_random_state(*this->rng_);
    cost_ = this->model_->calculate_cost(state_);
  }

  void make_step(uint64_t) override
  {
    Transition_T transition;
    double best = 1.0;
    for (int i = 0; i < samples_; i++)
    {
      Transition_T candidate =
          this->model_->get_random_transition(state_, *this->rng_);
      double diff = this->model_->calculate_cost_difference(state_, candidate);
      if (diff < best)
      {
        best = diff;
        transition = candidate;
      }
    }
    if (best <= 0.0)
    {
      this->model_->apply_transition(transition, state_);
      cost_ += best;
      DEBUG("new best: ", cost_, " ", state_);
      this->update_lowest_cost(cost_, state_);
    }
  }

  void finalize() override { this->update_lowest_cost(cost_, state_); }

 protected:
  int samples_;
  double cost_;
  State_T state_;
};

}  // namespace examples
