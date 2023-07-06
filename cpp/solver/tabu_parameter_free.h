
#pragma once
#include <cmath>

#include "utils/exception.h"
#include "utils/range.h"
#include "solver/parameter_free_adapter.h"
#include "solver/tabu.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
///  Defines a parameter-free tabu search solver.
/// The parameter free solver uses the TabuSolver class and tunes the two
/// parameters: tabu tenure and iterations (sweeps).
template <class Model_T>
class TabuParameterFree : public Tabu<Model_T>,
                          public ParameterFreeLinearSearchAdapterInterface
{
 public:
  TabuParameterFree()
      : tenure_fraction_denominator_(4), // Beasley (1998) - optimal tabu tenure
                                         // equal this fraction of variables count
        max_tabu_optimal_(20),           // Cap for optimal tabu tenure
        initial_sweeps_sample_points_({10, 20, 30, 50}) // Vector of initial sample points 
                                                        // for finding optimal sweeps number
  {
  }
  TabuParameterFree(const TabuParameterFree&) = delete;
  TabuParameterFree& operator=(const TabuParameterFree&) = delete;

  void configure(const utils::Json& json) override
  {
    Tabu<Model_T>::configure(json);
    using ::matcher::GreaterEqual;
    using ::matcher::GreaterThan;
    using ::matcher::LessEqual;
    auto& params = json[utils::kParams];
    this->param(params, "initial_sweeps_sample_points",
                initial_sweeps_sample_points_)
        .matches(SizeIs(GreaterThan(2)))
        .description("Vector of initial sample points for sweeps.");
    this->param(params, "min_num_restarts", min_num_restarts_)
        .matches((GreaterThan(0)))
        .default_value(this->get_thread_count())
        .description("Min number of restarts.");

    replica_rng_ = this->rng_->fork();

    this->restarts_ = min_num_restarts_;
    this->step_limit_ = initial_sweeps_sample_points_[0];

    this->set_output_parameter("restarts", this->restarts_);
    this->set_output_parameter("sweeps", this->step_limit_);
  }

  size_t parameter_dimensions() const override { return 2; }

  std::vector<int> parameter_ranges() const override
  {
    return initial_sweeps_sample_points_;
  }

  void update_parameters(const std::vector<double>& parameters,
                         double left_over_time) override
  {
    int sweeps = std::lround(parameters[0]);
    int sweeps_back = std::lround(parameters[1]);
    // Set paramenters for tabu
    this->step_limit_ = std::max(sweeps, 2);
    this->restarts_ = min_num_restarts_ * sweeps_back / sweeps;
    this->tabu_tenures_ = determine_all_tenures_(this->restarts_);
    this->set_output_parameter("restarts", this->restarts_);
    this->set_output_parameter("sweeps", this->step_limit_);
    this->reset(left_over_time);
  }

  double estimate_execution_cost() const override
  {
    size_t thread_count = this->get_thread_count();
    return double(this->step_limit_.value()) *
           double(this->target_number_of_states()) / double(thread_count);
  }

 private:
  size_t get_optimal_tenures_number() const
  {
    // Beasley (1998) - optimal tabu tenure
    auto num_variables = this->get_model().get_sweep_size();
    auto tabu_optimal = std::min(max_tabu_optimal_,
                                 num_variables / tenure_fraction_denominator_);
    return tabu_optimal;
  }

  std::vector<unsigned int> determine_all_tenures_(size_t n_tenures)
  {
    auto tabu_optimal = get_optimal_tenures_number();
    std::vector<unsigned int> tenures;
    tenures.reserve(n_tenures);

    auto num_variables = this->get_model().get_sweep_size();

    size_t tenures_to_create = n_tenures;

    // Need multiple folds if n_tenures is more then num_variables
    // to avoid use of tenures more then num_variables
    while (tenures_to_create > 0)
    {
      // Always create a solver with optimal tabu
      tenures.push_back(tabu_optimal);
      tenures_to_create--;

      // Check if creating one with half the optimal tabu life is needed
      if (tenures_to_create > 0 && tabu_optimal > max_tabu_optimal_ / 2)
      {
        tenures.push_back(tabu_optimal / 2);
        tenures_to_create--;
      }

      // Create the rest of the tenures across num_variables spread
      if (tenures_to_create > 0)
      {
        size_t tabu_increment =
            std::max((num_variables - tabu_optimal) /
                         (tenures_to_create * tenure_fraction_denominator_),
                     (size_t)1);

        size_t tenure = tabu_optimal + tabu_increment;
        while (tenures_to_create > 0 && tenure < num_variables)
        {
          tenures.push_back(tenure);
          tenures_to_create--;
          tenure += tabu_increment;
        }
      }
    }
    return tenures;
  }

  int min_num_restarts_;
  size_t tenure_fraction_denominator_;
  size_t max_tabu_optimal_;

  std::unique_ptr<::utils::RandomGenerator> replica_rng_;
  // Vector of initial sample points for sweeps
  std::vector<int> initial_sweeps_sample_points_;
};
}  // namespace solver