// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>

#include "utils/component.h"
#include "utils/config.h"
#include "utils/exception.h"
#include "utils/log.h"
#include "utils/operating_system.h"
#include "utils/optional.h"
#include "utils/random_generator.h"
#include "utils/qio_signal.h"
#include "utils/timing.h"
#include "matcher/matchers.h"
#include "observe/observable.h"
#include "observe/observer.h"
#include "omp.h"
#include "solver/evaluation_counter.h"
#include "solver/population.h"
#include "solver/solver.h"
#include "solver/solver_registry.h"

namespace solver
{
template <class Model_T>
class ModelSolver : public Solver
{
 public:
  using Base_T = Solver;
  using State_T = typename Model_T::State_T;
  using Cost_T = typename Model_T::Cost_T;

  ModelSolver() : model_(nullptr) {}
  ~ModelSolver() override {}

  void configure(const utils::Json& json) override
  {
    using ::matcher::GreaterThan;
    using ::matcher::LessEqual;

    if (model_ == nullptr)
    {
      THROW(utils::UnmetPreconditionException,
            "The model must be assigned to the solver before "
            "configuring it.");
    }

    Base_T::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kParams))
    {
      THROW(utils::MissingInputException, "Input field `", utils::kParams,
            "` is required.");
    }
    const utils::Json& params = json[utils::kParams];

    this->param(params, "number_of_solutions", solutions_to_return_)
        .description("number of solutions to return")
        .default_value(1u)
        .matches(GreaterThan(0u))
        .matches(LessEqual(10u));
    //
    // NOTE:
    //
    // .matches(LessEqual((unsigned)omp_get_max_threads()));
    //
    // Checking against omp_get_max_threads() here is not a good idea.
    // it will result in cryptic error messages to the users. We need
    // to specify the max number in the schema (or define it as best-effort,
    // up to the requested number).
    //
    // Also, the exception thrown here should be customized to explain
    // why the upper limit (especially if it's environment dependent).
    //
    // To the best of my knowledge, it isn't the number of solutions we
    // can currently return (e.g., SA + PT have multiple replicas per
    // thread).
  }

  virtual void set_model(const Model_T* model) { model_ = model; }

  utils::Structure get_model_properties() const override
  {
    return this->get_model().get_benchmark_properties();
  }

  size_t get_model_sweep_size() const override
  {
    return this->get_model().get_sweep_size();
  }

  size_t get_model_term_size() const
  {
    return this->get_model().get_term_count();
  }

  Cost_T get_lowest_cost() const
  {
    if (!lowest_cost_.has_value())
    {
      LOG(WARN, "accessing unset lowest cost");
      return NAN;
    }
    return *lowest_cost_;
  }

  utils::Structure get_solutions() const override
  {
    LOG_MEMORY_USAGE("start of getting solutions");
    utils::Structure s;
    s["version"] = "1.0";
    const Model_T& model_cur = this->get_model();
    if (lowest_cost_.has_value())
    {
      s["cost"] = *lowest_cost_ + model_cur.get_const_cost();
      s["configuration"] = model_cur.render_state(lowest_state_);

      // introduce this field even if multiple solutions were not requested. We
      // want to move customers into using this new array format.
      utils::Structure additional_s(utils::Structure::ARRAY);
      for (unsigned i = 0; i < lowest_states_.size(); i++)
      {
        utils::Structure sol;
        sol["cost"] = lowest_costs_[i] + model_cur.get_const_cost();
        sol["configuration"] = model_cur.render_state(*lowest_states_[i]);
        additional_s.push_back(sol);
      }

      s["solutions"] = additional_s;
    }
    else
    {
      if (is_empty())
      {
        // in case the model is empty just return the const cost.
        s["cost"] = model_cur.get_const_cost();
      }
    }
    s["parameters"] = this->get_output_parameters();
    LOG_MEMORY_USAGE("end of getting solutions");

#ifdef qiotoolkit_PROFILING
    this->add_profiling(s);
#endif
    return s;
  }

  virtual std::string init_memory_check_error_message() const = 0;

  virtual size_t target_number_of_states() const = 0;

  // Default memory estimation for solvers,
  // taking into account maximum number of states
  // solver is holding in the memory.
  // Override this method for more accurate estimation.

  virtual void init_memory_check()
  {
    const Model_T& model = this->get_model();
    // estimate memory needed for method
    size_t memory_per_state = model.state_memory_estimate();
    size_t memory_per_lower_state = model.state_only_memory_estimate();
    size_t memory_estimation =
        (memory_per_lower_state + memory_per_state) * target_number_of_states();
    size_t available_memory = utils::get_available_memory();
    if (available_memory < memory_estimation)
      throw utils::MemoryLimitedException(init_memory_check_error_message());
  }

  // Copy result from this solve to other
  // This is used for parameter solver to copy result from its worker solver.
  void copy_solutions_other(ModelSolver* other,
                            unsigned solutions_to_return) const
  {
    for (unsigned i = 0; i < solutions_to_return; i++)
    {
      other->lowest_costs_.push_back(lowest_costs_[i]);
      other->lowest_states_.push_back(lowest_states_[i]);
    }
  }

  void copy_lowest_state(ModelSolver* other) const
  {
    other->lowest_state_ = lowest_state_;
  }

  size_t count_solutions() const { return lowest_states_.size(); }

  virtual bool is_empty() const { return this->get_model().is_empty(); }

  virtual const Model_T& get_model() const { return *model_; }

 protected:
  virtual size_t max_replicas_of_state() const
  {
    const Model_T& model = this->get_model();
    // estimate memory needed for method
    size_t memory_per_state = model.state_memory_estimate();
    size_t memory_per_lower_state = model.state_only_memory_estimate();
    size_t available_memory = utils::get_available_memory();
    return available_memory / (memory_per_state + memory_per_lower_state);
  }

  virtual size_t max_replicas_adjusted_state(uint32_t max_replicas_mult) const
  {
    size_t max_states = this->get_thread_count() * max_replicas_mult;
    size_t states = max_replicas_of_state();
    // Our memory estimation is  just an estimation since the runtime memory
    // consumption is not deterministic. So here we reduce the estimation by a
    // factor to allow the system have enough leftover memory for model states
    // The following number are got from MaxSAT performance benchmark tests.
#ifdef _DEBUG
    double adjust = states * 0.5;
#else
    double adjust = states * 0.58;
#endif
    if (adjust < 1.)
    {
      // at least one state, if we OOM, the solver will throw OOM exception.
      return 1;
    }
    return std::min((size_t)adjust, max_states);
  }

  virtual size_t adjust_states(double input_states,
                               uint32_t max_replicas_mult) const
  {
    size_t max_states = this->max_replicas_adjusted_state(max_replicas_mult);
    size_t result = (size_t)input_states;
    result = std::min(result, max_states);
    if (max_states > (size_t)this->thread_count_)
    {
      result = std::max(result, (size_t)this->thread_count_);
    }
    else
    {
      result = max_states;
    }

    return result;
  }

  bool update_lowest_cost(Cost_T cost, const State_T& state)
  {
    if (!lowest_cost_.has_value() || cost < *lowest_cost_)
    {
      lowest_cost_ = cost;
      lowest_state_ = state;
      return true;
    }
    else
    {
      return false;
    }
  }

  template <typename Walker_T>
  void copy_solutions(std::vector<Walker_T>& walkers,
                      unsigned solutions_to_return)
  {
    std::partial_sort(walkers.begin(), walkers.begin() + solutions_to_return,
                      walkers.end(), Walker_T::compare);
    for (unsigned i = 0; i < solutions_to_return; i++)
    {
      lowest_costs_.push_back(walkers[i].get_lowest_cost());
      lowest_states_.push_back(&walkers[i].get_lowest_state());
    }
  }

  template <typename Walker_T>
  void copy_solutions(Population<Walker_T>& population,
                      unsigned solutions_to_return)
  {
    population.partial_sort(solutions_to_return);

    // Population may not always persist the global lowest.
    // Check if current lowest is the same as the current set of solutions
    if (lowest_cost_.has_value())
    {
      if (*lowest_cost_ < population[0]->get_lowest_cost())
      {
        lowest_costs_.push_back(*lowest_cost_);
        lowest_states_.push_back(&lowest_state_);
        solutions_to_return--;
      }
    }

    for (unsigned i = 0; i < solutions_to_return; i++)
    {
      lowest_costs_.push_back(population[i]->get_lowest_cost());
      lowest_states_.push_back(&population[i]->get_lowest_state());
    }
  }

  // Populates solutions including and in addition to the current best.
  // Will modify the input vector by partial sort.
  template <typename Solutions_T>
  void populate_solutions(Solutions_T& solutions)
  {
    unsigned solutions_to_return =
        std::min(solutions_to_return_, (unsigned)solutions.size());
    copy_solutions(solutions, solutions_to_return);
    this->update_lowest_cost(lowest_costs_[0], *lowest_states_[0]);
  }

#ifdef qiotoolkit_PROFILING
  virtual void add_profiling(utils::Structure& s) const
  {
    // Add memory usage information
    utils::Structure memory_profiling(utils::Structure::ARRAY);
    for (const auto& it : utils::MemoryUsageLog::get())
    {
      utils::Structure memory_point(utils::Structure::OBJECT);
      memory_point[it.first] = it.second;
      memory_profiling.push_back(memory_point);
    }
    s["profiling"]["memory_usage"] = memory_profiling;
  }
#endif

  Model_T* get_model_unconst()
  {
    return const_cast<Model_T*>(model_);
  }

  const Model_T* model_;
  std::optional<Cost_T> lowest_cost_;  // stores lowest cost
  State_T lowest_state_;

  // For returning multiple solutions.
  unsigned solutions_to_return_;
  std::vector<const State_T*> lowest_states_;
  std::vector<Cost_T> lowest_costs_;
};

}  // namespace solver
