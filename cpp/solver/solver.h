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
#include "utils/optional.h"
#include "utils/random_generator.h"
#include "utils/qio_signal.h"
#include "utils/timing.h"
#include "matcher/matchers.h"
#include "observe/observable.h"
#include "observe/observer.h"
#include "omp.h"
#include "solver/evaluation_counter.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// Interface for a Solver
///
/// A solver is initialized, run and then asked to print a result.
class Solver : public ::observe::Observer
{
 public:
  /// Intanstiate an uninitialized Solver
  Solver() : thread_count_(1), cost_milestones_("step", "cost") {}
  virtual ~Solver() {}

  /// Get the identifier of this solver.
  /// This identifier is denoted as the `target` in the request.
  virtual std::string get_identifier() const = 0;

  /// Initialize the solver.
  virtual void init() = 0;

  /// Run the solver.
  ///
  /// @see SteppingSolver
  virtual void run() = 0;

  /// Get the structured result description.
  virtual utils::Structure get_solutions() const = 0;
  virtual utils::Structure get_model_properties() const
  {
    utils::Structure s;
    s["type"] = model_type_;
    return s;
  }
  virtual utils::Structure get_solver_properties() const
  {
    utils::Structure s = Observer::render();

    if (time_limit_.has_value())
    {
      s["time_limit"] = time_limit_.value();
    }
     if (eval_limit_.has_value())
    {
       s["eval_limit"] = eval_limit_.value();
    }
    if (cost_limit_.has_value())
    {
      s["cost_limit"] = cost_limit_.value();
    }

    return s;
  }
  virtual size_t get_model_sweep_size() const = 0;

  utils::Structure get_benchmark() const
  {
    utils::Structure s;
    s["cost_function_evaluation_count"] =
        evaluation_counter_.get_function_evaluation_count();
    s["cost_difference_evaluation_count"] =
        evaluation_counter_.get_difference_evaluation_count();

    s[utils::kMaxLocality] = -1;
    s[utils::kMinLocality] = -1;
    s[utils::kAvgLocality] = -1;

    auto model_properties = get_model_properties();
    // Translate graph model properties to the output format
    if (model_properties.has_key("graph") &&
        model_properties["graph"].has_key("size"))
    {
      auto& size = model_properties["graph"]["size"];
      if (size.has_key("nodes"))
      {
        s[utils::kVariableCount] = size["nodes"];
      }
      if (size.has_key("edges"))
      {
        s[utils::kTermCount] = size["edges"];
      }

      if (size.has_key(utils::kAvgLocality))
      {
        s[utils::kAvgLocality] = size[utils::kAvgLocality];
      }

      if (size.has_key(utils::kMaxLocality))
      {
        s[utils::kMaxLocality] = size[utils::kMaxLocality];
      }

      if (size.has_key(utils::kMinLocality))
      {
        s[utils::kMinLocality] = size[utils::kMinLocality];
      }

      if (size.has_key(utils::kAccumDependentVars))
      {
        s[utils::kAccumDependentVars] = size[utils::kAccumDependentVars];
      }

      if (size.has_key(utils::kTotalLocality))
      {
        s[utils::kTotalLocality] = size[utils::kTotalLocality];
      }

      if (size.has_key(utils::kMaxCouplingMagnitude))
      {
        s[utils::kMaxCouplingMagnitude] = size[utils::kMaxCouplingMagnitude];
      }

      if (size.has_key(utils::kMinCouplingMagnitude))
      {
        s[utils::kMinCouplingMagnitude] = size[utils::kMinCouplingMagnitude];
      }
    }

    s.set_extension("counters", evaluation_counter_);
    s.set_extension("solver", get_solver_properties());
    s.set_extension("model", model_properties);
    return s;
  }

  utils::Structure get_result() const
  {
    utils::Structure s;
    s["benchmark"] = get_benchmark();
    s["solutions"] = get_solutions();
    return s;
  }

  /// Check the identifier and version against the configuraiton.
  virtual void configure(const utils::Json& json)
  {
    Observer::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kParams))
    {
      THROW(utils::MissingInputException, "Input field `", utils::kParams,
            "` is required.");
    }
    const utils::Json& params = json[utils::kParams];

    this->param(params, "time_limit", time_limit_)
        .description("Number of seconds the solver should run.")
        .alias("timeout")
        .with_output();
    if (time_limit_.has_value() && time_limit_.value() < 0)
    {
      throw utils::ValueException("timeout value must be non-negative");
    }
    this->param(params, "eval_limit", eval_limit_)
        .description(
            "Stop the solver after the cost function was evaluated this many "
            "times.")
        .with_output();

    this->param(params, "cost_limit", cost_limit_)
        .description(
            "Stop the solver if a value equal or lower to this is found.")
        .with_output();

    this->param(params, "threads", thread_count_)
        .description(
            "The number of threads to use in parallel sections of the code.")
        .default_value(0);

    if (thread_count_ > 0 && thread_count_ < omp_get_max_threads())
    {
      // only set up the thread count when thread_count_ is positive and less
      // than max number of cores (OMP default value) since it make no sense
      // to have more threads that cpu cores, as our algorithm is cpu
      // intensive.
      omp_set_num_threads(thread_count_);
    }
    else
    {
      thread_count_ = omp_get_max_threads();
    }
    // We currently don't want to surface
    // this->set_output_parameter("threads", thread_count_);

    if (params.HasMember(utils::kDisabledFeatures))
    {
      this->param(params, utils::kDisabledFeatures, disabled_features_)
          .description("features to be disabled");
      utils::set_disabled_feature(disabled_features_);
    }

    if (params.HasMember(utils::kEnabledFeatures))
    {
      this->param(params, utils::kEnabledFeatures, enabled_features_)
          .description("features to be enabled");
      utils::set_enabled_feature(enabled_features_);
    }
  }

  /// Return the number of threads which should be used in parallel sections
  /// of the solver.
  int get_thread_count() const { return thread_count_; }

  virtual void finalize()
  {
    // Finalize the sampling process, do nothing in abstract class
  }

  void copy_limits(Solver* other) const
  {
    other->time_limit_ = time_limit_;
    other->cost_limit_ = cost_limit_;
    other->eval_limit_ = eval_limit_;
  }

  void set_time_limit(double value) { time_limit_ = value; }

 protected:
  /// Return the maximum number of threads this solver can use.
  ///
  int get_max_threads() const { return omp_get_max_threads(); }
  std::set<int> disabled_features_;
  std::set<int> enabled_features_;

  std::string model_type_;
  std::optional<double> time_limit_;
  std::optional<uint64_t> eval_limit_;
  std::optional<double> cost_limit_;
  EvaluationCounter evaluation_counter_;
  int thread_count_;
  ::observe::Milestone cost_milestones_;
};

}  // namespace solver
