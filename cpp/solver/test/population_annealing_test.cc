// Copyright (c) Microsoft. All Rights Reserved.
#include "solver/population_annealing.h"

#include <cmath>
#include <set>
#include <sstream>
#include <string>

#include "utils/config.h"
#include "utils/json.h"
#include "utils/random_generator.h"
#include "utils/structure.h"
#include "gtest/gtest.h"
#include "model/ising.h"
#include "model/ising_grouped.h"
#include "model/pubo.h"
#include "model/pubo_grouped.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::utils::Structure;
using ::solver::PopulationAnnealing;

void reset_TestModel_consts()
{
  TestModel::kInitValue = 0.00001;
  TestModel::kBaseRandom = 0.0001;
  TestModel::kBaseDiff = 0.000005;
  TestModel::kBaseUniform = 0.001;
}

class PopulationAnnealingTest : public ::testing::Test
{
 public:
  void configure(const std::string& params_data)
  {
    reset_TestModel_consts();
    std::string input(R"({
      "cost_function": {
        "version": "1.0",
        "type": "toy"
      }
    })");
    utils::configure_with_configuration_from_json_string(input, toy_);
    toy_.init();
    auto params = utils::json_from_string(params_data);
    solver_.set_model(&toy_);
    solver_.configure(params);
    solver_.init();
  }

  utils::Structure run(const std::string& params_data)
  {
    configure(params_data);
    solver_.run();
    solver_.finalize();
    return solver_.get_result();
  }

 protected:
  TestModel toy_;
  PopulationAnnealing<TestModel> solver_;
};

TEST_F(PopulationAnnealingTest, SimulatesToyModelWithFrictionTensor)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 100,
      "threads": 1,
      "enabled_features":[2],
      "resampling_strategy": "friction_tensor",
      "friction_tensor_constant": 1,
      "number_of_solutions": 2
    }
  })");
  EXPECT_LT(result["solutions"]["cost"].get<double>(), 1e-4);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() -
                     TestModel::kInitValue),
            1e-4);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);
}

TEST_F(PopulationAnnealingTest, SimulatesToyModelWithEnergyVariance)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 100,
      "population": 128,
      "threads": 1,
      "enabled_features":[2],
      "resampling_strategy": "energy_variance"
    }
  })");
  EXPECT_LT(result["solutions"]["cost"].get<double>(), 1e-5);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() -
                     TestModel::kInitValue),
            1e-5);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(),
            1);  // default array size
}

TEST_F(PopulationAnnealingTest, SimulatesToyModelWithConstantCulling)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 100,
      "population": 128,
      "threads": 1,
      "resampling_strategy": "constant_culling",
      "culling_fraction": 0.2
    }
  })");
  EXPECT_LT(result["solutions"]["cost"].get<double>(), 1e-5);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() -
                     TestModel::kInitValue),
            1e-5);
}

TEST_F(PopulationAnnealingTest, SimulatesToyModelWithLinearSchedule)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 200,
      "threads": 1
    }
  })");
  EXPECT_LT(result["solutions"]["cost"].get<double>(), 1e-5);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() -
                     TestModel::kInitValue),
            1e-5);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["beta"]["final"].get<double>(), 5.);
  EXPECT_DOUBLE_EQ(result["solutions"]["parameters"]["alpha"].get<double>(),
                   2.);
}

TEST_F(PopulationAnnealingTest, LinearScheduleDecreasing)
{
  auto params_data = R"({
    "params": {
      "seed": 42,
      "threads": 1,
      "beta_start":4,
      "beta_stop":2,
      "resampling_strategy": "linear_schedule"
    }
  })";

  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST_F(PopulationAnnealingTest, SimulatesToyModelWithGeometricSchedule)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 200,
      "threads": 1,
      "beta_start":0.99,
      "beta_stop":1.99,
      "resampling_strategy": "geometric_schedule"
    }
  })");
  EXPECT_LT(result["solutions"]["cost"].get<double>(), 1e-5);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() -
                     TestModel::kInitValue),
            1e-5);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["beta"]["initial"].get<double>(), 0.99);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["beta"]["final"].get<double>(), 1.99);
  EXPECT_DOUBLE_EQ(result["solutions"]["parameters"]["alpha"].get<double>(),
                   2.);
}

TEST_F(PopulationAnnealingTest, GeometricScheduleZero)
{
  auto params_data = R"({
    "params": {
      "seed": 42,
      "step_limit": 200,
      "threads": 1,
      "beta_start":0,
      "beta_stop":2,
      "resampling_strategy": "geometric_schedule"
    }
  })";

  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST_F(PopulationAnnealingTest, GeometricScheduleDecreasing)
{
  auto params_data = R"({
    "params": {
      "seed": 42,
      "threads": 1,
      "beta_start":4,
      "beta_stop":2,
      "resampling_strategy": "geometric_schedule"
    }
  })";

  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST_F(PopulationAnnealingTest, PopulationIsZero)
{
  auto params_data = R"({
    "params": {
      "seed": 42,
      "step_limit": 200,
      "threads": 1,
      "population": 0,
      "enabled_features":[2],
      "resampling_strategy": "constant_culling",
      "culling_fraction": 0.000001
    }
  })";
  EXPECT_THROW_MESSAGE(
      configure(params_data), utils::ValueException,
      "parameter `population`: must be greater than 0, found 0");
}

TEST_F(PopulationAnnealingTest, SimulatesInvalidMultipleSolutionValue)
{
  auto params_data = R"({
    "params": {
      "seed": 42,
      "population": 10,
      "number_of_solutions": 3000000
    }
  })";
  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST(PopulationAnnealing, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  PopulationAnnealing<::model::PuboWithCounter<uint64_t>> pa;
  auto result = run_solver(pa, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> pubo;
  PopulationAnnealing<::model::PuboGrouped<uint64_t>> pa;
  auto result = run_solver(pa, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  PopulationAnnealing<::model::IsingTermCached> pa;
  auto result = run_solver(pa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped ising;
  PopulationAnnealing<::model::IsingGrouped> pa;
  auto result = run_solver(pa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  PopulationAnnealing<::model::PuboWithCounter<uint64_t>> pa;
  auto result = run_solver(pa, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  PopulationAnnealing<::model::IsingTermCached> pa;
  auto result = run_solver(pa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, PuboEmptyMixedModel)
{
  std::set<int> disabled = {2};
  utils::set_disabled_feature(disabled);
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  PopulationAnnealing<::model::PuboWithCounter<uint64_t>> pa;
  auto result = run_solver(pa, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(PopulationAnnealing, IsingEmptyMixedModel)
{
  std::set<int> disabled = {2};
  utils::set_disabled_feature(disabled);
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  PopulationAnnealing<::model::IsingTermCached> pa;
  auto result = run_solver(pa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(PopulationAnnealing, ConstPuboGroupedModel)
{
  std::string input_file(utils::data_path("constpubogrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> model;
  PopulationAnnealing<::model::PuboGrouped<uint64_t>> pa;
  auto result = run_solver(pa, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(PopulationAnnealing, ConstIsingGroupedModel)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped model;
  PopulationAnnealing<::model::IsingGrouped> pa;
  auto result = run_solver(pa, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}