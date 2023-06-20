// Copyright (c) Microsoft. All Rights Reserved.
#include "strategy/bayesian.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/file.h"
#include "utils/json.h"
#include "utils/random_generator.h"
#include "externals/gpp/gpp_common.hpp"
#include "externals/gpp/gpp_covariance.hpp"
#include "externals/gpp/gpp_domain.hpp"
#include "externals/gpp/gpp_logging.hpp"
#include "externals/gpp/gpp_math.hpp"
#include "externals/gpp/gpp_model_selection.hpp"
#include "externals/gpp/gpp_optimizer_parameters.hpp"
#include "externals/gpp/gpp_random.hpp"
#include "gtest/gtest.h"
#include "omp.h"
#include "strategy/gd_parameters.h"

using strategy::BayesianOpt;
using strategy::GDParameters;
TEST(Bayesian, RangeTests)
{
  GDParameters model_parameters;
  model_parameters.num_multistarts_ = omp_get_max_threads();
  model_parameters.max_num_steps_ = 100;
  model_parameters.max_num_restarts_ = 12;
  model_parameters.num_steps_averaged_ = 4;
  model_parameters.max_mc_steps_ = 100;
  model_parameters.gamma_ = 1.01;
  model_parameters.pre_mult_ = 1.0e-3;
  model_parameters.max_relative_change_ = 1.0;
  model_parameters.tolerance_ = 1.0e-6;
  model_parameters.num_to_sample_ = 1;

  GDParameters search_parameters;
  search_parameters.num_multistarts_ = omp_get_max_threads();
  search_parameters.max_num_steps_ = 100;
  search_parameters.max_num_restarts_ = 12;
  search_parameters.gamma_ = 0.7;
  search_parameters.pre_mult_ = 1.0;
  search_parameters.max_relative_change_ = 0.8;
  search_parameters.tolerance_ = 1.0e-7;
  search_parameters.num_steps_averaged_ = 4;
  search_parameters.max_mc_steps_ = 100;
  search_parameters.num_to_sample_ = 5;
  BayesianOpt bayesian;
  bayesian.init(2, 88, model_parameters, search_parameters);
  std::vector<std::pair<double, double>> ranges = {{0.1, 20.}, {2., 2000.}};

  std::vector<double> param1 = {200., 100.};
  bayesian.add_new_sample(param1, -9.0);
  std::vector<double> param2 = {20., 200.};
  bayesian.add_new_sample(param2, -1.0);

  std::vector<double> param3 = {50., 6.};
  bayesian.add_new_sample(param3, -1000.0);

  std::vector<double> param4 = {40., 5.};
  bayesian.add_new_sample(param4, -600.0);
  bayesian.set_ranges(ranges);
  for (int i = 0; i < search_parameters.num_to_sample_; i++)
  {
    std::vector<double> new_param;
    bayesian.recommend_parameter_values(new_param);
    ASSERT_EQ(2, new_param.size());
    EXPECT_LE(new_param[0], ranges[0].second);
    EXPECT_LE(ranges[0].first, new_param[0]);
    EXPECT_LE(new_param[1], ranges[1].second);
    EXPECT_LE(ranges[1].first, new_param[1]);
  }

  utils::Structure s;
  bayesian.get_perf_metrics(s);
  EXPECT_LT(0, s["model_training_ms"].get<int64_t>());
  EXPECT_LT(0, s["total_training_ms"].get<int64_t>());
}

TEST(Bayesian, SavedSamplesTests)
{
  GDParameters model_parameters;
  model_parameters.num_multistarts_ = omp_get_max_threads();
  model_parameters.max_num_steps_ = 100;
  model_parameters.max_num_restarts_ = 12;
  model_parameters.num_steps_averaged_ = 4;
  model_parameters.max_mc_steps_ = 100;
  model_parameters.gamma_ = 1.01;
  model_parameters.pre_mult_ = 1.0e-3;
  model_parameters.max_relative_change_ = 1.0;
  model_parameters.tolerance_ = 1.0e-6;
  model_parameters.num_to_sample_ = 1;

  GDParameters search_parameters;
  search_parameters.num_multistarts_ = omp_get_max_threads();
  search_parameters.max_num_steps_ = 100;
  search_parameters.max_num_restarts_ = 12;
  search_parameters.gamma_ = 0.7;
  search_parameters.pre_mult_ = 1.0;
  search_parameters.max_relative_change_ = 0.8;
  search_parameters.tolerance_ = 1.0e-7;
  search_parameters.num_steps_averaged_ = 4;
  search_parameters.max_mc_steps_ = 100;
  search_parameters.num_to_sample_ = 5;
  size_t saved = 8;
  BayesianOpt bayesian;
  bayesian.init(3, 188, model_parameters, search_parameters, saved);
  std::vector<double> sample = {0., 2., 3.};
  for (size_t i = 0; i < saved * 4; i++)
  {
    sample[0] = (double)i;
    bayesian.add_new_sample(sample, i * (-1.));
  }
  EXPECT_EQ(saved, bayesian.num_of_saved_samples());
  std::vector<double> read_sample;
  double obj = bayesian.get_sample(0, read_sample);
  EXPECT_EQ(24.0, read_sample[0]);
  EXPECT_EQ(-24.0, obj);
  sample[0] = -1.0;
  bayesian.add_new_sample(sample, 100.);
  obj = bayesian.get_sample(0, read_sample);
  EXPECT_EQ(-1., read_sample[0]);
  EXPECT_EQ(100., obj);
}

using namespace optimal_learning;

namespace strategy
{
std::unique_ptr<GaussianProcess> create_model(
    double tolerance, std::vector<double>& trained_parameters, int dim,
    std::vector<double>& sample_points, std::vector<double>& sample_objectives,
    std::vector<int>& derivatives, utils::RandomGenerator& rng);
}

TEST(GPP, CreateModelNull)
{
  double tolerance = 100;
  int dim = 7;
  std::vector<double> trained_parameters(dim + 2, 1.0);
  std::vector<double> sample_points;
  std::vector<double> sample_objectives;
  std::vector<int> derivatives(dim, 0);
  ::utils::Twister rng;
  rng.seed(288);
  for (int i = 0; i < dim; i++)
  {
    derivatives[i] = i;
  }
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < dim; j++)
    {
      sample_points.push_back(2.0);
    }
    sample_objectives.push_back(-100);
  }
  std::unique_ptr<GaussianProcess> model =
      strategy::create_model(tolerance, trained_parameters, dim, sample_points,
                             sample_objectives, derivatives, rng);
  EXPECT_NE(nullptr, model.get());
  sample_points.clear();
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < dim; j++)
    {
      sample_points.push_back(2.0 + i * j);
    }
  }
  model = strategy::create_model(0, trained_parameters, dim, sample_points,
                                 sample_objectives, derivatives, rng);
  EXPECT_NE(nullptr, model.get());
}
