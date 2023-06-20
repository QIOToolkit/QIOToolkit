// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "utils/component.h"
#include "utils/config.h"
#include "utils/exception.h"
#include "utils/random_generator.h"
#include "markov/state.h"
#include "markov/transition.h"
#include "model/base_model.h"
#include "model/model_registry.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// Interface for Markov models
///
/// Implementations of the Markov model interface must provide
///
///   1) A `calculate_cost` (to calculate the cost of an entire state as well as
///      the `calculate_cost_difference` incurred by a specific transition).
///
///   2) The means to generate the Monte Carlo chain
///      (`random_state`, `random_transition` and `apply_transition`).
///
/// The interface is templated for both the underlying Markov state and
/// transition, such that users can decide how to represent each. The
/// base classes markov::State and markov::Transition are provided as guidelines
/// for designing these model-specifc types, but they are not enforced to
/// be used. (That is, you may use Model<std::string, int> if a string is
/// sufficient to represent your state and an int defines a
/// transition.
///
/// @see State
/// @see Transition
///
/// For initial testing, you may also opt to use the simplified classes
/// `markov::SimpleTransition` and `markov::SimpleModel`.
///
/// @see SimpleModel
/// @see SimpleTransition
///
template <class State, class Transition, class Cost = double>
class Model : public ::model::BaseModel
{
 public:
  using Base_T = ::model::BaseModel;
  using State_T = State;
  using Transition_T = Transition;
  using Cost_T = Cost;

  /// Create an uninitialized model.
  Model() : step_limit_(0) {}

  void set_step_limit(uint64_t limit)
  {
    step_limit_ = limit;
  }

  void configure(const utils::Json& json) override { Base_T::configure(json); }

  void configure(model::BaseModelConfiguration& configuration)
  {
    Base_T::configure(configuration);
  }

  void init() override { Base_T::init(); }

  /// Definition of the cost function.
  ///
  /// Evaluate the entire cost function for the state being passed. For
  /// instance, in the case of a model from statistical mechanics, this would be
  /// the Hamiltonian.
  virtual Cost_T calculate_cost(const State_T& state) const = 0;

  /// Partial evaluation of the cost function.
  ///
  /// This method should calculate the difference in cost if we move from
  /// `state` (=before) to the one resulting from applying `transition` to
  /// `state` (=after):
  ///
  /// \f[ \Delta_{C} = C_{\mathrm{after}} - C_{\mathrm{before}} \f]
  ///
  /// In code:
  ///
  ///   State state = get_random_state(rng);
  ///   Transition transition = get_random_transition(rng);
  ///   double cost_before = calculate_cost(state);
  ///   double cost_diff = calculate_cost_difference(state, transition);
  ///   apply_transition(transition, state);  // modify state
  ///   double cost_after = calculate_cost(state);
  ///   // before + diff should correspond to `after` (up to double precision)
  ///   assert(cost_before + cost_diff == cost_after);
  ///
  virtual Cost_T calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const = 0;

  /// Return a valid random state for the model.
  ///
  /// The various algorithms don't know how to initialize a valid state; this
  /// allows them to start with a random one. NOTE: The
  /// `rng` being passed should be used for randomness (as
  /// opposed to creating one for the model), as multiple threads can
  /// potentially use the same underlying model (albeit with separate
  /// rngs).
  ///
  virtual State_T get_random_state(utils::RandomGenerator& rng) const = 0;

  /// Check if model has initial configuration
  virtual bool has_initial_configuration() const { return false; }

  /// Return a state from initial configuration
  virtual State_T get_initial_configuration_state() const 
  {
    throw utils::NotImplementedException(
        "get_initial_configuration_state is not implemented yet");
  };

  /// Return a random transition starting at state.
  ///
  /// The various algorithms don't know how to move throuhg markov space; this
  /// allows them to pick a random direction given a starting point.
  ///
  ///   1) By definition, the next markov state should only depend on the
  ///      current one, so only the last one is passed to this function.
  ///
  ///   2) "random" does not necessitate equi-distributed here (you may choose
  ///      a different distribution if your model dictates it). However, the
  ///      the typical choise is to pick randomly from all possible moves at
  ///      state with equal probability.
  ///
  virtual Transition_T get_random_transition(
      const State_T& state, utils::RandomGenerator& rng) const = 0;

  /// Apply a transition to a state.
  ///
  /// This changes the configuration represented by `*state`.
  /// Depending on the optimization algorithm, transition can either be applied
  /// conditionally or alaways (e.g., population dynamics). Separating the
  /// functionality into the three interfaces `random_transition`,
  /// `calculate_cost_difference`, `apply_transition` leaves control over the
  /// strategy with the optimization method.
  virtual void apply_transition(const Transition& transition,
                                State& state) const = 0;

  /// Render a state of this model
  ///
  /// Default implementation of how a state of this model is rendered (by'
  /// invoking the state's proper rendering mechanic). Overloading this method
  /// allows a model implementation to customize how a state is printed. This
  /// is relevant if the rendering needs to depend on model parameters in
  /// addition to the state.
  virtual utils::Structure render_state(const State_T& state) const
  {
    utils::Structure rendered = state;
    return rendered;
  }

  /// Return the number of (attempted) transition to consider `one sweep`.
  ///
  /// This number is expected to scale roughly with the number of variables
  /// in the model such that, on average, each variable is selected once
  /// per sweep (if selected randomly).
  virtual size_t get_sweep_size() const { return 1; }

  // Return number of term entries in the model
  virtual size_t get_term_count() const
  {
    throw utils::NotImplementedException(
        "get_term_count is not implemented yet");
  }

  /// Collect statistics about the model being simulated.
  ///
  /// The output of this method will appear in the
  /// `Response.benchmark.input_data` field. By default, we do not collect
  /// any model statistics -- implementations of this interface must override
  /// this method to fill the output.
  virtual utils::Structure get_benchmark_properties() const
  {
    return utils::Structure();
  }

  utils::Structure render() const override { return utils::Structure(); }

  virtual bool is_rescaled() const { return false; }
  virtual void rescale() {}
  virtual Cost_T get_scale_factor() const { return 1.0; }

  virtual Cost_T get_const_cost() const { return 0; }

  virtual bool is_empty() const = 0;

  /// Estimate memory consumption of model state
  /// and lowest state in bytes,
  /// using paramters known by model.
  /// Numerous states will be created by solvers.
  /// Estimation of memory consumption is needed to avoid memory overflow.
  /// States may have complex nature, so knowledge of model parameters
  /// should be used for accurate estimation.
  /// For prototype models return 0 could be used.
  virtual size_t state_memory_estimate() const = 0;

  virtual size_t state_only_memory_estimate() const = 0;

  virtual double estimate_max_cost_diff() const
  {
    throw utils::NotImplementedException(
        "estimate_max_cost_diff is not implemented yet");
  }

  virtual double estimate_min_cost_diff() const
  {
    throw utils::NotImplementedException(
        "estimate_min_cost_diff is not implemented yet");
  }
  protected:
  uint64_t step_limit_;
};

////////////////////////////////////////////////////////////////////////////////
/// Simplified Model representation
///
/// This base class has a predifined transition type (`SimpleTransition`) and
/// implementations for `calculate_cost_difference` and `apply_transition`.
/// These implementations make no assumptions about the model and they are, as a
/// result, typically inefficient (i.e., calculating the whole calculate_cost
/// in lieu of only the changed parts).
///
/// The base class is provided as a means for rapid prototyping because it
/// requires less interfaces to be implemented.
///
template <class State>
class SimpleModel : public Model<State, SimpleTransition<State>>
{
 public:
  using State_T = State;
  using Transition_T = SimpleTransition<State>;
  using Cost_T = double;

  std::string get_identifier() const override = 0;
  std::string get_version() const override = 0;

  /// Default calculate_cost_difference implementation.
  ///
  /// This computes the expected difference of a transition by invoking
  /// calculate_cost twice. This is almost always inefficient (at the very
  /// least, one could cache the value for the starting point). `SimpleModel`
  /// therefore only serves the purpose of rapidly writing a base
  /// implementation to check against.
  ///
  /// NOTE: This relies on using SimpleTransition with the model - such
  /// that both the start and end state of the transition can easily be
  /// accessed.
  Cost_T calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override
  {
    return calculate_cost(transition.target()) - calculate_cost(state);
  }

  /// Default apply_transition implementation.
  ///
  /// Since SimpleTransition has Transition as a base, we can just use
  /// its apply method (which, in turn, will set state to `to()`).
  virtual void apply_transition(const Transition_T& transition,
                                State_T& state) const override
  {
    transition.apply(state);
  }
};

/// Base class for implementations which wish to have full control over how
/// an entire sweep is performed. Implementations should
///
/// 1) Defer to accept(delta_cost) -> bool when deciding whether to apply
///    a proposed change (this is typically the metropolis acceptance rate)
/// 2) Call check_lowest() whenever a potentially new lowest cost is found
///    (check_lowest, as opposed to save_lowest, does store the state as
///     new best only if its cost is lower than the current best)
/// 3) Keep "cost" updated to the one corresponding to the current state
///    (at least before every call to check_lowest() and after the sweep
///     is completed).
template <class State>
class LinearSweepModel : public Model<State, size_t>
{
 public:
  virtual void make_linear_sweep(
      double& cost, State& state, std::function<bool(double)> accept,
      std::function<void(void)> check_lowest) const = 0;
};

}  // namespace markov
