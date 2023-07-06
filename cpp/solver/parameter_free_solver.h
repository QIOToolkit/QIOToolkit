
#pragma once
#include "utils/structure.h"
#include "solver/stepping_solver.h"
#include "strategy/bayesian.h"

namespace solver
{
template <class Model_T, class SolverAdapter,
          class StrategyType = strategy::BayesianOpt>
class ParameterFreeSolver : public SteppingSolver<Model_T>
{
 public:
  ParameterFreeSolver()
      : stable_bests_(0), initial_samples_(4), nonimprovement_limit_(8)
  {
  }

  ParameterFreeSolver(const ParameterFreeSolver&) = delete;
  ParameterFreeSolver& operator=(const ParameterFreeSolver&) = delete;
  ~ParameterFreeSolver() {}

  /// Identifier of this solver (`target` in the request)
  std::string get_identifier() const override
  {
    std::string worker_str = solver_worker_.get_identifier();
    size_t pos = worker_str.rfind(".");
    return worker_str.substr(0, pos) + "-parameterfree" +
           worker_str.substr(pos, worker_str.length() - pos);
  }

  void init() override
  {
    strategy_.init(solver_worker_.parameter_dimensions(), this->seed_);

    // to ensure timeout checking as fast as possible
    this->fixed_step_per_tick(1);
  }

  std::string init_memory_check_error_message() const override
  {
    return solver_worker_.init_memory_check_error_message();
  }

  virtual size_t target_number_of_states() const override
  {
    return solver_worker_.target_number_of_states();
  }
  /// Read the maximum number of steps from configuration.
  void configure(const utils::Json& json) override
  {
    solver_worker_.configure(json);

    solver_worker_.copy_limits(this);
    this->seed_ = solver_worker_.get_seed();
    if (!this->time_limit_.has_value())
    {
      // time out must be set.
      this->set_time_limit(5.);
      solver_worker_.set_time_limit(5.);
    }
    training_parameters(json);
  }

  void make_step(uint64_t step) override
  {
    std::vector<double> parameters;
    parameters.reserve(solver_worker_.parameter_dimensions());
    if (step < initial_samples_)
    {
      // need to run a few runs first before strategy can make meaningful
      // compuation
      solver_worker_.get_initial_parameter_values(parameters);
    }
    else
    {
      auto ranges = solver_worker_.parameter_ranges();
      strategy_.set_ranges(ranges);
      bool found = strategy_.recommend_parameter_values(parameters);
      if (!found)
      {
        // In case the strategy could not find a group of parameters for search
        // we fall back to linear by changing the best set of parameters' steps
        // or replicas
        if (stable_bests_ < nonimprovement_limit_)
        {
          LOG(WARN,
              "Search strategy failed to update a new set of parameters, doing "
              "linear update");
          parameters = parameters_best_;
          solver_worker_.update_parameters_linearly(parameters);
        }
        else
        {
          LOG(INFO,
              "Not witness any gains for mutliple runs, probably get the best "
              "result, exits");
          this->set_time_limit(0);
          return;
        }
      }
    }
    double left_over_time =
        this->time_limit_.value() - (utils::get_wall_time() - this->start_time_);
    if (left_over_time <= 0 && this->lowest_cost_.has_value())
    {
      LOG(INFO, "Stop parameter searching, becauseof timeout:",
          this->time_limit_.value());
      return;
    }
    solver_worker_.update_parameters(parameters, left_over_time);

    solver_worker_.init();

    // solver shall own the step limit set up by parameters
    solver_worker_.fixed_step_per_tick(1);
    solver_worker_.run();

    // collect useful run time information
    solver_worker_.update_accumulated_info();

    solver_worker_.finalize();

    double objective = solver_worker_.get_lowest_cost();
    strategy_.add_new_sample(parameters, objective);

    if (this->lowest_cost_.has_value() &&
        (std::abs(this->lowest_cost_.value() - objective) < 10E-12))
    {
      stable_bests_++;

      // in case that the lowest energy found are the same, we save the
      // parameters whose estimated execution cost is the least.
      double cur_execution_cost = solver_worker_.estimate_execution_cost();
      if (cur_execution_cost < execution_time_best_)
      {
        update_best(objective, parameters, cur_execution_cost);
      }
    }

    if (stable_bests_ >= nonimprovement_limit_)
    {
      LOG(INFO,
          "Not witness any gains for mutliple runs, probably get the best "
          "result, exits");
      this->set_time_limit(0);
    }

    if (!this->lowest_cost_.has_value() ||
        this->lowest_cost_.value() - objective >
            10E-12)  // ignore the possible floating point calculation error
    {
      stable_bests_ = 0;

      update_best(objective, parameters,
                  solver_worker_.estimate_execution_cost());
    }
  }

  void finalize() override
  {
    if (this->is_empty())
    {
      return;
    }
    unsigned solutions_to_return = (unsigned)solver_worker_.count_solutions();

    this->lowest_costs_.reserve(solutions_to_return);
    this->lowest_states_.reserve(solutions_to_return);
    this->lowest_costs_.push_back(this->lowest_cost_.value());
    this->lowest_states_.push_back(&this->lowest_state_);

    solutions_to_return--;
    solver_worker_.copy_solutions_other(this, solutions_to_return);

    // update with the best parameters found.
    solver_worker_.update_parameters(parameters_best_, 0);
  }

  utils::Structure get_model_properties() const override
  {
    return solver_worker_.get_model_properties();
  }

  utils::Structure get_solver_properties() const override
  {
    auto properties = solver_worker_.get_solver_properties();
    utils::Structure training_perf;
    strategy_.get_perf_metrics(training_perf);
    properties["training"] = training_perf;
    return properties;
  }

  utils::Structure get_output_parameters() const override
  {
    return solver_worker_.get_output_parameters();
  }

  const Model_T& get_model() const override
  {
    return solver_worker_.get_model();
  }

  void set_model(const Model_T* model) override
  {
    ModelSolver<Model_T>::set_model(model);
    solver_worker_.set_model(model);
  }

 protected:
  virtual void training_parameters(const utils::Json& parameters)
  {
    using ::matcher::GreaterThan;
    auto& params = parameters[utils::kParams];

    strategy_.configure(params, this->get_thread_count());

    this->param(params, strategy::kInitialSamples, initial_samples_)
        .description("initial number samples to be provided for model training")
        .matches(GreaterThan<uint32_t>(0));
    this->param(params, strategy::kNonImprovementLimit, nonimprovement_limit_)
        .description("number samples witeness no improvement for termination")
        .matches(GreaterThan<uint32_t>(0));
  }

#ifdef qiotoolkit_PROFILING
  void add_profiling(utils::Structure& s) const override
  {
    SteppingSolver<Model_T>::add_profiling(s);
    strategy_.add_profiling(s);
  }
#endif
 protected:
  void update_best(double objective, std::vector<double>& parameters,
                   double estimated_execution_cost)
  {
    this->lowest_cost_ = objective;
    solver_worker_.copy_lowest_state(this);
    parameters_best_ = parameters;
    execution_time_best_ = estimated_execution_cost;
  }
  // The actual solver class which will execute solver algorithm
  SolverAdapter solver_worker_;
  // Parameter searching strategy class like BayesianOpt
  StrategyType strategy_;
  // Best set of parameters found sor far
  std::vector<double> parameters_best_;
  // estimated execution time of current best parameter.
  double execution_time_best_;

  // number of times we did not find a better set of results
  uint32_t stable_bests_;
  // Number of initial samples to be provided for searching and model building
  uint32_t initial_samples_;
  // Max number of non-improved searching result for existing
  uint32_t nonimprovement_limit_;
};
}  // namespace solver