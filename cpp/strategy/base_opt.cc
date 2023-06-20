// Copyright (c) Microsoft. All Rights Reserved.
#include <assert.h>

#include <chrono>
#include <cmath>
#include <sstream>

#include "utils/log.h"
#include "omp.h"
#include "strategy/bayesian.h"

namespace strategy
{
void BaseOpt::get_perf_metrics(utils::Structure& perf) const
{
  perf["model_training_ms"] = model_time_ms_;
  perf["total_training_ms"] = learning_time_ms_;
}

void BaseOpt::log_parameters(const std::string& header,
                             const std::vector<double>& parameters)
{
  std::stringstream ss;
  for (size_t i = 0; i < parameters.size(); i++)
  {
    ss << parameters[i] << " ";
  }
  LOG(INFO, header, ss.str());
}

#ifdef qiotoolkit_PROFILING
void BaseOpt::update_profile(int64_t model_ms, int64_t learning_ms)
{
  profile_modeling_ms_.push_back(model_ms);
  profile_learning_ms_.push_back(learning_ms);
}

void BaseOpt::update_objective_profile(double objective,
                                       const std::vector<double>& parameters)
{
  profile_sample_objectives_.push_back(objective);
  for (size_t i = 0; i < parameters.size(); i++)
  {
    profile_sample_points_.push_back(parameters[i]);
  }
}

void BaseOpt::add_profiling(utils::Structure& s) const
{
  utils::Structure learning_ms(utils::Structure::ARRAY);
  learning_ms.append(profile_learning_ms_);
  s["profiling"]["learning_ms"] = learning_ms;

  utils::Structure model_ms(utils::Structure::ARRAY);
  model_ms.append(profile_modeling_ms_);
  s["profiling"]["modeling_ms"] = model_ms;

  utils::Structure objectives(utils::Structure::ARRAY);
  objectives.append(profile_sample_objectives_);
  s["profiling"]["objectives"] = objectives;

  utils::Structure parameters(utils::Structure::ARRAY);
  for (size_t i = 0; i < profile_sample_objectives_.size(); i++)
  {
    utils::Structure params(utils::Structure::ARRAY);
    for (size_t j = 0; j < dimensions_; j++)
    {
      params.push_back(profile_sample_points_[i * dimensions_ + j]);
    }
    parameters.push_back(params);
  }
  s["profiling"]["parameters"] = parameters;
}
#endif
}  // namespace strategy
