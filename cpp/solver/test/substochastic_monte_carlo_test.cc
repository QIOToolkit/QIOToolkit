// Copyright (c) Microsoft. All Rights Reserved.
#include "solver/substochastic_monte_carlo.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/file.h"
#include "utils/json.h"
#include "utils/random_generator.h"
#include "gtest/gtest.h"
#include "model/ising.h"
#include "model/ising_grouped.h"
#include "model/pubo.h"
#include "model/pubo_grouped.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::solver::SubstochasticMonteCarlo;

class SubstochasticMonteCarloTest : public ::testing::Test
{
 public:
  void configure(const std::string& params_data)
  {
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
  SubstochasticMonteCarlo<TestModel> solver_;
};

TEST_F(SubstochasticMonteCarloTest, SimulatesToyModel)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "target_population": 100,
      "step_limit": 1000,
      "alpha": {
        "type": "linear",
        "start": 200,
        "stop": 800,
        "initial": 0.8,
        "final": 0.2
      },
      "beta": {
        "type": "linear",
        "start": 200,
        "stop": 800,
        "initial": 0.2,
        "final": 0.8
      },
      "number_of_solutions": 2
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_EQ(
      result["solutions"]["parameters"]["target_population"].get<size_t>(),
      100);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["alpha"]["initial"].get<double>(), 0.8);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["beta"]["final"].get<double>(), 0.8);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);
}

TEST_F(SubstochasticMonteCarloTest, SimulatesInvalidMultipleSolutionValue)
{
  auto params_data = R"({
    "params": {
      "step_limit": 100,
      "target_population": 100,
      "step_limit": 1000,
      "number_of_solutions": 3000000
    }
  })";
  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST(SubstochasticMonteCarlo, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  SubstochasticMonteCarlo<model::PuboWithCounter<uint64_t>> ssmc;
  auto result = run_solver(ssmc, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-12, result["solutions"]["cost"].get<double>());
}

TEST(SubstochasticMonteCarlo, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  SubstochasticMonteCarlo<model::PuboWithCounter<uint64_t>> ssmc;
  auto result = run_solver(ssmc, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(SubstochasticMonteCarlo, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  SubstochasticMonteCarlo<::model::IsingTermCached> ssmc;
  auto result = run_solver(ssmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(SubstochasticMonteCarlo, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  SubstochasticMonteCarlo<::model::IsingTermCached> ssmc;
  auto result = run_solver(ssmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(SubstochasticMonteCarlo, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  SubstochasticMonteCarlo<model::PuboWithCounter<uint64_t>> ssmc;
  auto result = run_solver(ssmc, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(SubstochasticMonteCarlo, PuboMixedConstantTermModel)
{
  std::string input_file(utils::data_path("mixedconstants.json"));
  std::string param_file(utils::data_path("paramsnull.json"));
  ::model::PuboWithCounter<uint8_t> pubo;
  SubstochasticMonteCarlo<model::PuboWithCounter<uint8_t>> ssmc;
  auto result = run_solver(ssmc, pubo, input_file, param_file);
  EXPECT_GT(-500, result["solutions"]["cost"].get<double>());
}

TEST(SubstochasticMonteCarlo, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  SubstochasticMonteCarlo<::model::IsingTermCached> ssmc;
  auto result = run_solver(ssmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(SubstochasticMonteCarlo, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> pubo;
  SubstochasticMonteCarlo<::model::PuboGrouped<uint64_t>> ssmc;
  auto result = run_solver(ssmc, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(SubstochasticMonteCarlo, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped ising;
  SubstochasticMonteCarlo<::model::IsingGrouped> ssmc;
  auto result = run_solver(ssmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

TEST(SubstochasticMonteCarlo, ConstPuboGroupedModel)
{
  std::string input_file(utils::data_path("constpubogrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> model;
  SubstochasticMonteCarlo<::model::PuboGrouped<uint64_t>> ssmc;
  auto result = run_solver(ssmc, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(SubstochasticMonteCarlo, ConstIsingGroupedModel)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped model;
  SubstochasticMonteCarlo<::model::IsingGrouped> ssmc;
  auto result = run_solver(ssmc, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}