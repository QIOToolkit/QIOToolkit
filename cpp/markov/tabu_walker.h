// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <cmath>
#include <map>
#include <vector>
#include <type_traits>

#include "utils/utils.h"
#include "markov/hill_climbing_walker.h"
#include "markov/walker.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// Tabu walker

template <class Model>
class TabuWalker : public Walker<Model>
{
 public:
  using State = typename Model::Model::State_T;
  using Transition = typename Model::Model::Transition_T;

  /// Create a TabuWalker instance with uninitialized model and state.
  TabuWalker()
      : tabu_sweep_iterations_(0),
        tabu_tenure_(0),
        local_search_mode_(false),
        incomplete_sweep_iterations_(0)
  {
  }

  void init() override
  {
    Walker<Model>::init();
    clear_tabu_list();
    local_search_mode_ = false;
    tabu_sweep_iterations_ = 0;
    incomplete_sweep_iterations_ = 0;
  }

  /// Decide whether to accept a given cost increase.
  bool accept(const typename Model::Cost_T& cost_diff) override
  {
    bool accepted = this->cost_ + cost_diff < this->lowest_cost_;
    return accepted;
  }

  /// Estimate memory consumption using model parameters
  static size_t memory_estimate(const Model& model)
  {
    return sizeof(TabuWalker<Model>) - sizeof(Walker<Model>) -
           sizeof(HillClimbingWalker<Model>) +
           Walker<Model>::memory_estimate(model) +
           HillClimbingWalker<Model>::memory_estimate(model);
  }

  void make_sweep()
  {
    if (local_search_mode_)
    {
      local_search_sweep();
    }
    else
    {
      // Search neighbours for best flip
      std::optional<typename Model::Cost_T> best_cost_diff;
      typename Model::Transition_T best_transition;
      bool transition_accepted = false;

      size_t sweep_size = this->model_->get_sweep_size();

      for (size_t i = 0; i < sweep_size; i++)
      {
        auto transition = get_transition(i);

        auto cost_before = this->cost_;
        auto lowest_cost_before = this->lowest_cost_;
        
        auto cost_diff = this->attempt_transition(transition);

        transition_accepted = cost_before + cost_diff < lowest_cost_before;

        if (transition_accepted)
        {
          // Add to tabu (or refresh)
          tabu_list_[transition] = tabu_sweep_iterations_ + tabu_tenure_;
          local_search_init();

          
          // Track and compensate incomplete tabu sweeps for accurate sweep count
          incomplete_sweep_iterations_ += i;
          if (incomplete_sweep_iterations_ < sweep_size)
          {
            // Compensate incomplete tabu sweeps
            local_search_sweep();
          }
          else
         {
            // Skip compensation when incomplete iterations
            // enough for full sweep
            incomplete_sweep_iterations_ -= sweep_size;
          }

          break;
        }

        // If item on tabu list, skip
        if (tabu_list_[transition] >= tabu_sweep_iterations_)
        {
          continue;
        }

        if (!best_cost_diff.has_value() || cost_diff < best_cost_diff.value())
        {
          best_cost_diff = cost_diff;
          best_transition = transition;
        }
      }

      if (!transition_accepted && best_cost_diff.has_value())
      {
        // apply the best transition
        this->apply_transition(best_transition, best_cost_diff.value());
        tabu_list_[best_transition] = tabu_sweep_iterations_ + tabu_tenure_;
      }
      tabu_sweep_iterations_++;
    }
  }

  /// Set tabu tenure parameter
  void set_tenure(unsigned int tabu_tenure) { tabu_tenure_ = tabu_tenure; }

 private:

  using TabuListType = typename std::conditional<
      std::is_same<typename Model::Transition_T, size_t>::value,
      std::vector<size_t>,
      std::unordered_map<typename Model::Transition_T, size_t>>::type;

  template <class TT = typename Model::Transition_T>
  typename std::enable_if<std::is_same<TT, size_t>::value, void>::type
  clear_tabu_list(void)
  {
    tabu_list_.resize(this->model_->get_sweep_size(), 0);
  }

  template <class TT = typename Model::Transition_T>
  typename std::enable_if<!std::is_same<TT, size_t>::value, void>::type
  clear_tabu_list(void)
  {
    tabu_list_.clear();
  }

  template <class TT = typename Model::Transition_T>
  typename std::enable_if<std::is_same<TT, size_t>::value, TT>::type
  get_transition(size_t i)
  {
    return i;
  }

  template <class TT = typename Model::Transition_T>
  typename std::enable_if<!std::is_same<TT, size_t>::value, TT>::type
  get_transition(size_t)
  {
    return this->model_->get_random_transition(this->state_, *this->rng_);
  }

  void local_search_init()
  {
    // copy state
    local_search_mode_ = true;
    hill_climber_.set_model(this->model_);
    hill_climber_.set_rng(this->rng_);
    hill_climber_.init(this->state_);
  }

  void local_search_sweep()
  {

    auto prev_cost = hill_climber_.get_lowest_cost();
    auto prev_counter = hill_climber_.get_evaluation_counter();
    
    hill_climber_.make_sweep();
    
    // update statistics
    auto delta_counter = hill_climber_.get_evaluation_counter();
    
    delta_counter -= prev_counter;
    this->evaluation_counter_ += delta_counter;

    auto curr_cost = hill_climber_.get_lowest_cost();
    if (curr_cost < prev_cost)
    {
      // update result if better soulution was found
      this->lowest_cost_ = hill_climber_.get_lowest_cost();
      this->lowest_state_.copy_state_only(hill_climber_.get_lowest_state());
    }
    else
    {
      local_search_mode_ = false;
    }
  }


  size_t tabu_sweep_iterations_;
  TabuListType tabu_list_;  // The tabu list stores the expiry
                            // iteration
                            // of all variables.
  unsigned int tabu_tenure_;
  HillClimbingWalker<Model> hill_climber_;

  bool local_search_mode_;
  size_t incomplete_sweep_iterations_;
};

}  // namespace markov
