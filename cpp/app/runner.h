// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <memory>
#include <string>

#include "utils/dimacs.h"
#include "utils/json.h"
#include "markov/model.h"
#include "model/max_sat.h"
#include "solver/solver.h"

////////////////////////////////////////////////////////////////////////////////
/// Generic qiotoolkit runner
///
/// This class instantiate any solver + model combination and runs the
/// simulation according to the configuration specified in json.
///
/// Example:
///
///   ```cpp
///   Runner runner;
///   runner.configure(R"(
///     {
///       "target": "...",
///       ...
///     }
///   )");
///   runner.run();
///   ```

namespace app
{
class Runner
{
 public:
  Runner()
      : output_benchmark_(true),
        input_size_bytes_(0),
        configure_time_ms_(0.0),
        configure_cputime_ms_(0.0)
  {
  }
  Runner(const Runner&) = delete;
  Runner& operator=(const Runner&) = delete;
  virtual ~Runner() {}

  void set_output_benchmark(bool value) { output_benchmark_ = value; }
  void set_input_file(std::string file_path) { input_file_ = file_path; }
  void set_parameter_file(std::string file_path)
  {
    parameter_file_ = file_path;
  }
  virtual void set_solver(std::string target) { target_ = target; }
  std::string get_target() { return this->target_; }

  void configure();

  /// Configure the solver and model to run.
  ///
  /// This parses the string being passed as json and selects the solver and
  /// model according to the `target` and `input_data.type` attributes defined
  /// therein. In particular, it calls `Solver::configure()` and
  /// `Model::configure()` with a `Json` config, meaning that any work
  /// performed as part of reading the input arguments will happen here (this
  /// includes setting up the graph for `GraphModel`s).
  ///
  /// NOTE: `Response.benchmark.end2end_runtime_ms` in the benchmark output
  /// will reflect the time / spent in this function PLUS the time spent in
  /// `Runner::run()`.
  void configure(const std::string& json);

  /// Run the simulation.
  ///
  /// This calls `Solver::init()` followed by `Solver::run()`.
  ///
  /// NOTE: `Response.benchmark.execution_time_ms` will reflect the time spent
  /// in this method (excluding `Structure::to_string()` converting the
  /// response to string, because that happens after the time measurement is
  /// written into the response)
  virtual std::string run();

  virtual utils::Structure get_run_output();

  ::solver::Solver* get_solver() const { return solver_.get(); }

#ifdef _DEBUG
  void set_test_configure(std::unique_ptr<::solver::Solver>& solver,
                          const std::string& input_file,
                          const std::string& param_file,
                          const std::string& target, const std::string& model)
  {
    solver_.swap(solver);
    input_file_ = input_file;
    parameter_file_ = param_file;
    target_ = target;
    model_type_ = model;
  }
#endif

 protected:
  void copy_if_present(const std::string& key, const utils::Structure& src,
                       utils::Structure& target);
  void add_common_metrics(utils::Structure& benchmarks,
                          utils::Structure& metrics);

  std::unique_ptr<::solver::Solver> solver_;
  bool output_benchmark_;

 private:
  virtual void configure(const std::string& input, const utils::Json& parameters,
                         const std::string& solver_name);

  /// Handle dimacs input for max-sat.
  void configure(const utils::Dimacs& dimacs, const utils::Json& parameters,
                 const std::string& solver_name);

  /// Select a suitable implementation (i.e., Counter_T size) for
  /// the max sat model.
  void select_max_sat_implementation(::model::MaxSat32* maxsat,
                                     const utils::Json& params,
                                     const std::string& target);

  /// Check that a solver has been created successfully.
  ///
  /// Otherwise, throw an exception indicating whether model selection
  /// or solver selection failed.
  void check_solver(const std::string& model_type,
                    const std::string& selected_model,
                    const std::string& target);

  virtual void metric_to_console(utils::Structure& metrics);

  // Whether the memory saving is enabled for current runner
  bool memory_saving_enabled() const;

  // Whether we can retry to use the memory saving model
  bool memory_saving_retry() const;

  // Reset the runner for memory saving.
  void reset_for_memory_saving();

  bool inline target_support_memory_saving() const
  {
    return target_ != "microsoft.substochasticmontecarlo-parameterfree.cpu" &&
           target_ != "microsoft.substochasticmontecarlo.cpu";
  }

  bool inline model_support_memory_saving() const
  {
    return model_type_ == "ising" || model_type_ == "pubo";
  }

  std::string input_file_;
  std::string parameter_file_;
  std::string target_;
  std::string model_type_;

  /// Private (owned) pointer to the solver instantiated
  /// (e.g. an `solver::ParallelTempering<model::Ising>`)
  std::unique_ptr<::model::BaseModel> model_;
  int input_size_bytes_;
  double configure_time_ms_;
  double configure_cputime_ms_;
};

}  // namespace app
