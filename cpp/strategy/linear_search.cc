
#include "strategy/linear_search.h"

#include <assert.h>

#include <chrono>
#include <cmath>
#include <sstream>

#include "utils/config.h"
#include "utils/log.h"
#include "omp.h"

namespace strategy
{

void LinearSearchOpt::set_ranges(const std::vector<int>& intial_sample_points)
{
  //use for initial set up only
  if (sample_points_.size() != 0) return;
  sample_points_ = intial_sample_points;
  sample_objectives_.assign(sample_points_.size(),
                            std::numeric_limits<double>::max());
  initial_minimum_sample_point_ = sample_points_[0];
}

void LinearSearchOpt::fit_sample_points()
{
  int lowest = std::distance(
      sample_objectives_.begin(),
      std::min_element(sample_objectives_.begin(), sample_objectives_.end()));
  if (lowest == 0)
  {
    if (sample_points_[lowest] > initial_minimum_sample_point_)
    {
      for (auto& point : sample_points_)
      {
        point = int(double(point) / search_interval_change_factor_);
      }
    }
  }
  else if (lowest == ((std::int64_t)sample_points_.size()) - 1)
  {
    for (auto& point : sample_points_)
    {
      point = int(double(point) * search_interval_change_factor_);
    }
  }

  sample_objectives_.assign(sample_points_.size(),
                            std::numeric_limits<double>::max());
}


bool LinearSearchOpt::recommend_parameter_values(std::vector<double>& parameters)
{
  auto start_time = std::chrono::high_resolution_clock::now();
  sample_cur_++;
  if (sample_cur_ >= int(sample_points_.size()))
  {
    sample_cur_ = 0;
    fit_sample_points();
  }

  if (this->dimensions_ == 1)
  {
    parameters.resize(1);
    parameters[0] = sample_points_[sample_cur_];
  }
  else if (this->dimensions_ == 2)
  {
    parameters.resize(2);
    parameters[0] = sample_points_[sample_cur_];
    parameters[1] = sample_points_.back();
  }
  else 
  {
    THROW(utils::ValueException,
          "Dimentions for Linear Search expected to be 1 or 2, observed: ",
          this->dimensions_);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();
  this->learning_time_ms_ += ms_diff;
  this->model_time_ms_ += ms_diff;
#ifdef qiotoolkit_PROFILING
  update_profile(ms_diff, ms_diff);
#endif
  return true;
}

// Feedback a new set of parameters and objective value to learning process
void LinearSearchOpt::add_new_sample(std::vector<double>& parameters,
                                   double objective)
{
  if (std::lround(parameters[0]) == sample_points_[sample_cur_])
    sample_objectives_[sample_cur_] = objective;
  else
    throw utils::NotImplementedException(
        "Search for parameters is not implemented.");
#ifdef qiotoolkit_PROFILING
  update_objective_profile(objective, parameters);
#endif
}

void LinearSearchOpt::configure(const utils::Json& json, int)
{
  auto& params = json[utils::kParams];
  this->param(params, "search_interval_change_factor",
              search_interval_change_factor_)
      .description("Stretching factor for search interval.");
}
void LinearSearchOpt::init(size_t dimensions, uint32_t, size_t)
{
  this->dimensions_ = dimensions;
}

}  // namespace strategy
