// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <algorithm>
#include <cassert>
#include "utils/exception.h"
#include "utils/operating_system.h"

#include <functional>
#include <type_traits>
#include <vector>

#include "utils/component.h"
#include "utils/random_generator.h"
#include "utils/random_selector.h"
#include "utils/self_consistency.h"
#include "markov/model.h"
#include "model/graph_model.h"
#include "solver/evaluation_counter.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// `ClusterWalker` is an abstract base class for quantum like space explorers
///
/// A ClusterWalker stores multiple state and costs and has pointers to
/// a model and random number generator to make steps. It is specifically
/// designed to allow copy assignment (assuming the Model::State_T does),
/// such that it can be duplicated in population markovs.
///
/// The Walker provides a base implementation of the step function, which
///
///   * proposes a random new states,
///   * computes the cost differences,
///   * uses virtual method to decide whether to create cluster binded
///   * variables from multiple states
///   * invokes a pure virtual method to decide whether to accept, and
///   * modifies the internal states using cluster if true is returned.
///
/// From this we can implement a `QuantumWalker` by returning acceptance rate
/// based on quantum transverse field.
///
///
template <class Model>
class ClusterWalker : public utils::Component
{
 public:
  /// The `rng` and `model` must be set before the
  /// object can be used. `cost` is only intialized after a call to `init()`
  ClusterWalker()
      : model_(nullptr),
        lowest_cost_(std::numeric_limits<double>::max())
  {
  }

  /// Must be called after the `rng_` and `model_` have
  /// been set. It initializes the walker to a random state and precomputes
  /// its current cost.
  ///
  /// NOTE: This is typically the only direct call to `calculate_cost`; from
  /// here the current `cost_` is updated from the computed difference at
  /// each step.
  virtual void init(size_t n_states = 1)
  {
    n_states_ = n_states;
    states_.resize(n_states_);
    costs_.resize(n_states_);
    clusters_.resize(n_states_);
    accepted_transitions_.resize(n_states_);
    cost_diffs_.resize(n_states_);
  }

  void init_state(size_t state_index, ::utils::RandomGenerator& rng)
  {
    states_[state_index] = model_->has_initial_configuration()
                               ? model_->get_initial_configuration_state()
                               : model_->get_random_state(rng);
    costs_[state_index] = model_->calculate_cost(states_[state_index]);
  }

  /// Implementations of the walker interface must provide a method to decide
  /// which steps to accept/reject. Currently this is only based on
  /// `cost_difference`, but the state or transition could be added.
  virtual bool accept(const typename Model::Cost_T& cost_difference,
                      ::utils::RandomGenerator& rng) = 0;

  /// Implementations of the walker interface must provide a method to decide
  ///  which cluster to accept/reject.
  virtual bool accept_cluster(::utils::RandomGenerator& rng) = 0;

  /// For models with a 'size_t' transition type, do an **ordered** sweep
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<std::is_same<TT, size_t>::value, void>::type
  get_transition(size_t variable_index,
                 typename Model::Transition_T& transition,
                 utils::RandomGenerator&)
  {
    transition = variable_index;
  }

  /// For any other transition type, do a **random** sweep
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<!std::is_same<TT, size_t>::value, void>::type
  get_transition(size_t, typename Model::Transition_T& transition,
                 utils::RandomGenerator& rng)
  {
    transition = model_->get_random_transition(states_[0], rng);
  }

  /// Provide a pointer to the (non-owned) model to use for cost calculations
  /// and proposing new steps.
  void set_model(const Model* model) { model_ = model; }

  /// Return a constant reference to the model being used.
  const Model& model() { return *model_; }

  solver::EvaluationCounter get_evaluation_counter()
  {
    return evaluation_counter_;
  }

  void add_difference_evaluations(size_t number_evals)
  {
    evaluation_counter_.difference_evaluations_ += number_evals;
  }

  void add_function_evaluations(size_t number_evals)
  {
    evaluation_counter_.function_evaluations_ += number_evals;
  }

  void reset_evaluation_counter() { evaluation_counter_.reset(); }

  typename Model::Cost_T get_lowest_cost() const { return lowest_cost_; }

  const typename Model::State_T& get_lowest_state() const
  {
    return lowest_state_;
  }

  /// Estimate memory consumtion using model parameters
  static size_t memory_estimate(const Model& model, size_t n_states)
  {
    return sizeof(ClusterWalker<Model>) +
           // States memory estimate
           n_states * (model.state_memory_estimate() -
                       sizeof(typename Model::State_T)) +
           // Solution memory estimate
           (model.state_only_memory_estimate() -
            sizeof(typename Model::State_T)) +
           // Costs memory estimate
           utils::vector_values_memory_estimate<typename Model::Cost_T>(
               n_states) +
           // Costs diffs memory estimate
           utils::vector_values_memory_estimate<typename Model::Cost_T>(
               n_states) +
           // Clusters memory estimate
           utils::vector_values_memory_estimate<bool>(n_states) +
           // Accepted transitions memory estimate
           utils::vector_values_memory_estimate<bool>(n_states);
  }

  // Comparator for sorting walkers
  static bool compare(const ClusterWalker<Model>& w1,
                      const ClusterWalker<Model>& w2)
  {
    return w1.lowest_cost_ < w2.lowest_cost_;
  }

  const std::vector<typename Model::State_T>& get_states() { return states_; }

  const std::vector<typename Model::Cost_T>& get_costs() { return costs_; }

  size_t size() { return n_states_; }

  /// Clusters are defined as boolean vector size of trotter, 
  /// where false - is start of new cluster,
  /// true - mean connected to previous cluster
  /// Example of 4 clusters, 8 trotter: 
  /// 1 1 1 2 2 3 4 4    - as bool vector:
  /// f t t f t f f t
  using ClustersType = std::vector<bool>;

  void try_form_clusters(size_t state_index,
                         const typename Model::Transition_T& transition,
                         utils::RandomGenerator& rng)
  {
    clusters_[state_index] = false;
    if (state_index != 0)
    {
      bool same_spin = same_state_value(states_[state_index - 1],
                                        states_[state_index], transition);
      if (same_spin)
      {
        bool accepted = accept_cluster(rng);
        clusters_[state_index] = accepted;
      }
    }
  }

  void try_flip_clusters(size_t state_index, utils::RandomGenerator& rng)
  {
    bool is_cluster_start = this->clusters_[state_index] == false;
    if (is_cluster_start)
    {
      double cluster_cost_diff = cost_diffs_[state_index];
      for (size_t i = state_index + 1; i < n_states_ && clusters_[i]; i++)
      {
        cluster_cost_diff += cost_diffs_[i];
      }

      bool is_accepted = accept(cluster_cost_diff, rng);

      accepted_transitions_[state_index] = is_accepted;
      for (size_t i = state_index + 1; i < n_states_ && clusters_[i]; i++)
      {
        accepted_transitions_[i] = is_accepted;
      }
    }
  }

  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      !std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  calculate_cost_diff(size_t state_index,
                      const typename Model::Transition_T& transition)
  {
    cost_diffs_[state_index] =
        model_->calculate_cost_difference(states_[state_index], transition);
  }

  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  calculate_cost_diff(size_t state_index,
                      const typename Model::Transition_T& transition)
  {
    cost_diffs_[state_index] = model_->calculate_cost_difference(
        states_[state_index], transition, &costs_[state_index]);
  }

  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      !std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  apply_transition(size_t state_index,
                   const typename Model::Transition_T& transition)
  {
    if (accepted_transitions_[state_index])
    {
      model_->apply_transition(transition, states_[state_index]);
      costs_[state_index] += cost_diffs_[state_index];
    }
  }

  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  apply_transition(size_t state_index,
                   const typename Model::Transition_T& transition)
  {
    if (accepted_transitions_[state_index])
    {
      model_->apply_transition(transition, states_[state_index],
                               &costs_[state_index]);
      costs_[state_index] += cost_diffs_[state_index];
    }
  }

  /// Store the current lowest state as the lowest
  void save_lowest()
  {
    size_t lowest_cost_index =
        std::min_element(costs_.begin(), costs_.end()) - costs_.begin();

    if (lowest_cost_index >= costs_.size()) return;

    if (costs_[lowest_cost_index] < lowest_cost_)
    {
      lowest_cost_ = costs_[lowest_cost_index];
      lowest_state_.copy_state_only(states_[lowest_cost_index]);
    }
  }

 protected:

  const Model* model_;

  size_t n_states_;
  std::vector<typename Model::State_T> states_;
  std::vector<typename Model::Cost_T> costs_;
  std::vector<typename Model::Cost_T> cost_diffs_;
  ClustersType clusters_;
  std::vector<bool> accepted_transitions_;

  typename Model::Cost_T lowest_cost_;
  typename Model::State_T lowest_state_;
  solver::EvaluationCounter evaluation_counter_;
};

}  // namespace markov
