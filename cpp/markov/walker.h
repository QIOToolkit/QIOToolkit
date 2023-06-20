// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <functional>

#include "utils/component.h"
#include "utils/random_generator.h"
#include "utils/self_consistency.h"
#include "markov/model.h"
#include "model/graph_model.h"
#include "solver/evaluation_counter.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// `Walker` is an abstract base class for configuration space explorers
///
/// A walker container stores its current state and cost and has pointers to
/// a model and random number generator to make steps. It is specifically
/// designed to allow copy assignment (assuming the Model::State_T does),
/// such that it can be duplicated in population markovs.
///
/// The Walker provides a base implementation of the step function, which
///
///   * proposes a random new state,
///   * computes the cost difference,
///   * invokes a pure virtual method to decide whether to accept, and
///   * modifies the internal state if true is returned.
///
/// From this we can implement a `RandomWalker` by always returning true
/// and a `Metropolis` walker by returning the Boltzmann acceptance rate.
///
/// @see markov::Metropolis
///
template <class Model>
class Walker : public utils::Component
{
 public:
  /// The `rng` and `model` must be set before the
  /// object can be used. `cost` is only intialized after a call to `init()`
  Walker() : rng_(nullptr), model_(nullptr), cost_(0) {}

  /// Must be called after the `rng_` and `model_` have
  /// been set. It initializes the walker to a random state and precomputes
  /// its current cost.
  ///
  /// NOTE: This is typically the only direct call to `calculate_cost`; from
  /// here the current `cost_` is updated from the computed difference at
  /// each step.
  virtual void init()
  {
    state_ = model_->has_initial_configuration()
                 ? model_->get_initial_configuration_state()
                 : model_->get_random_state(*rng_);
    cost_ = model_->calculate_cost(state_);
    save_lowest();
    evaluation_counter_.function_evaluations_++;
  }

  /// Must be called after the `model_` has been set. Initializes the walker
  /// to a specific state and precomputes its current cost.
  virtual void init(const typename Model::State_T& state)
  {
    state_ = state;
    cost_ = model_->calculate_cost(state_);
    save_lowest();
    evaluation_counter_.function_evaluations_++;
  }

  /// Implementations of the walker interface must provide a method to decide
  /// which steps to accept/reject. Currently this is only based on
  /// `cost_difference`, but the state or transition could be added.
  virtual bool accept(const typename Model::Cost_T& cost_difference) = 0;

  /// Verify cost difference does a sanity check on the calculated energy
  /// difference by evaluating the whole cost function and comparing it to
  /// the cached value. This is *very* costly and happens only in DEBUG builds,
  /// not in RELEASE builds.
  ///
  /// NOTE: An exception thrown by this sanity check means that the
  /// implementations of `calculate_cost` and `calculate_cost_difference`
  /// are not compatible (or, less likely, the model or state have been
  /// modified since the total cost was cached).
  bool verify_cost_difference(const typename Model::Cost_T& returned_difference)
  {
    auto actual_difference = model_->calculate_cost(state_) - cost_;
    evaluation_counter_.function_evaluations_++;
    if (std::abs(returned_difference - actual_difference) >= 1e-9)
    {
      LOG(FATAL, "calculate_cost_difference() is wrong: returned ",
          returned_difference, ", actual ", actual_difference);
      return false;
    }
    return true;
  }

  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  apply_transition(const typename Model::Transition_T& transition,
                   typename Model::Cost_T cost_diff)
  {
    model_->apply_transition(transition, state_, &cost_);
    // `self_consistency_assert` is only evaluated in SelfConsistency builds
    // (it's a macro that expands to nothing in other builds).
    self_consistency_assert(verify_cost_difference(cost_diff));
    cost_ += cost_diff;
    check_lowest();
  }

  /// For all other models, no extra cost parameter is needed.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      !std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  apply_transition(const typename Model::Transition_T& transition,
                   typename Model::Cost_T cost_diff)
  {
    model_->apply_transition(transition, state_);
    // `self_consistency_assert` is only evaluated in SelfConsistency builds
    // (it's a macro that expands to nothing in other builds).
    self_consistency_assert(verify_cost_difference(cost_diff));
    cost_ += cost_diff;
    check_lowest();
  }

  /// For models derived from markov::LinearSweepModel, let the model handle
  /// the entire sweep.
  template <class TM = Model, class TS = typename Model::State_T>
  typename std::enable_if<
      std::is_base_of<markov::LinearSweepModel<TS>, TM>::value, void>::type
  make_sweep()
  {
    model_->make_linear_sweep(
        cost_, state_,
        std::bind(&Walker<Model>::accept, this, std::placeholders::_1),
        std::bind(&Walker<Model>::check_lowest, this));
  }

  /// For models with a 'size_t' transition type, do an **ordered** sweep
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      std::is_same<TT, size_t>::value &&
          !std::is_base_of<markov::LinearSweepModel<TS>, TM>::value,
      void>::type
  make_sweep()
  {
    for (size_t i = 0; i < model_->get_sweep_size(); i++) attempt_transition(i);
  }

  /// For any other transition type, do a **random** sweep
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      !std::is_same<TT, size_t>::value &&
          !std::is_base_of<markov::LinearSweepModel<TS>, TM>::value,
      void>::type
  make_sweep()
  {
    for (size_t i = 0; i < model_->get_sweep_size(); i++) make_step();
  }

  /// Perform multiple sweeps.
  void make_sweeps(size_t n_sweeps)
  {
    for (size_t k = 0; k < n_sweeps; k++) make_sweep();
  }

  /// Perform a single (random) step
  void make_step()
  {
    auto transition = model_->get_random_transition(state_, *rng_);
    attempt_transition(transition);
  }

  /// Attempt the given transition and apply it if accepted by the walker
  /// condition.
  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value,
      typename Model::Cost_T>::type
  attempt_transition(const typename Model::Transition_T& transition)
  {
    typename Model::Cost_T cost_diff =
        model_->calculate_cost_difference(state_, transition, &cost_);
    evaluation_counter_.difference_evaluations_++;
    if (accept(cost_diff))
    {
      model_->apply_transition(transition, state_, &cost_);
      evaluation_counter_.accepted_transitions_++;
      // `self_consistency_assert` is only evaluated in SelfConsistency builds
      self_consistency_assert(verify_cost_difference(transition, cost_diff));
      cost_ += cost_diff;
      check_lowest();
    }
    return cost_diff;
  }

  /// Attempt the given transition and apply it if accepted by the walker
  /// condition.
  /// For all other models, no extra cost parameter is needed.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      !std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value,
      typename Model::Cost_T>::type
  attempt_transition(const typename Model::Transition_T& transition)
  {
    typename Model::Cost_T cost_diff =
        model_->calculate_cost_difference(state_, transition);
    evaluation_counter_.difference_evaluations_++;
    if (accept(cost_diff))
    {
      model_->apply_transition(transition, state_);
      evaluation_counter_.accepted_transitions_++;
      // `self_consistency_assert` is only evaluated in SelfConsistency builds
      self_consistency_assert(verify_cost_difference(transition, cost_diff));
      cost_ += cost_diff;
      check_lowest();
    }
    return cost_diff;
  }

  /// Return the cost of the current state (cached within the walker).
  const typename Model::Cost_T& cost() const { return cost_; }

  /// Return the current state of this walker.
  const typename Model::State_T& state() const { return state_; }

  /// Provide a pointer to the (non-owned) model to use for cost calculations
  /// and proposing new steps.
  void set_model(const Model* model) { model_ = model; }

  /// Provide a pointer to the (non-owned) random number generator to use.
  /// Multithreading must either be handled via mutex within the random number
  /// generator or by the calling class.
  void set_rng(utils::RandomGenerator* rng) { rng_ = rng; }

  /// Return a constant reference to the model being used.
  const Model& model() { return *model_; }

  /// Return a reference to the random number generator to use.
  utils::RandomGenerator& rng() { return *rng_; }

  /// Swap the state with another instance.
  ///
  /// This exchanges the internal `markov::State` and the associated
  /// (cached) `cost_` with another metropolis instance.
  void swap_state(Walker* other)
  {
    std::swap(state_, other->state_);
    std::swap(cost_, other->cost_);
  }

  solver::EvaluationCounter get_evaluation_counter()
  {
    return evaluation_counter_;
  }
  void reset_evaluation_counter() { evaluation_counter_.reset(); }

  /// Unconditionally store the current state as the lowest (during init)
  void save_lowest()
  {
    lowest_cost_ = cost_;
    lowest_state_.copy_state_only(state_);
  }

  /// Save the current state as new best if its cost is lower than the previous
  /// best.
  void check_lowest()
  {
    if (cost_ < lowest_cost_) save_lowest();
  }

  typename Model::Cost_T get_lowest_cost() const { return lowest_cost_; }

  // Applies any scale constants to the current cost.
  void apply_scale_factor(double scale_factor) { lowest_cost_ *= scale_factor; }

  const typename Model::State_T& get_lowest_state() const
  {
    return lowest_state_;
  }

  /// Estimate memory consumption using model parameters
  static size_t memory_estimate(const Model& model)
  {
    return sizeof(Walker<Model>) +
           (model.state_memory_estimate() - sizeof(typename Model::State_T)) +
           (model.state_only_memory_estimate() -
            sizeof(typename Model::State_T)) +
           2 * sizeof(typename Model::Cost_T);
  }
  // Comparator for sorting walkers
  static bool compare(const Walker<Model>& w1, const Walker<Model>& w2)
  {
    return w1.lowest_cost_ < w2.lowest_cost_;
  }

 protected:
  utils::RandomGenerator* rng_;
  const Model* model_;
  typename Model::State_T state_;
  typename Model::Cost_T cost_;
  typename Model::Cost_T lowest_cost_;
  typename Model::State_T lowest_state_;
  solver::EvaluationCounter evaluation_counter_;
};

}  // namespace markov
