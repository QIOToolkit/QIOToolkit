
#include "solver/tabu.h"

#include <cmath>
#include <set>
#include <sstream>
#include <string>

#include "utils/config.h"
#include "utils/json.h"
#include "utils/random_generator.h"
#include "utils/structure.h"
#include "model/ising.h"
#include "model/ising_grouped.h"
#include "model/pubo.h"
#include "model/pubo_grouped.h"
#include "gtest/gtest.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::utils::Structure;
using ::solver::Tabu;

void reset_TestModel_consts()
{
  TestModel::kInitValue = 0.00001;
  TestModel::kBaseRandom = 0.0001;
  TestModel::kBaseDiff = 0.000005;
  TestModel::kBaseUniform = 0.001;
}

class TabuTest : public ::testing::Test
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
  Tabu<TestModel> solver_;
};

TEST_F(TabuTest, SimulatesToyModel)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "restarts": 4,
      "step_limit": 100,
      "threads": 1,
      "tabu_tenure": 1,
      "number_of_solutions": 2
    }
  })");
  EXPECT_LT(result["solutions"]["cost"].get<double>(), 1e-4);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() -
                     TestModel::kInitValue),
            1e-4);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);
}


TEST_F(TabuTest, SimulatesInvalidMultipleSolutionValue)
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

TEST(Tabu, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  Tabu<::model::PuboWithCounter<uint64_t>> solver;
  auto result = run_solver(solver, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> pubo;
  Tabu<::model::PuboGrouped<uint64_t>> solver;
  auto result = run_solver(solver, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  Tabu<::model::IsingTermCached> solver;
  auto result = run_solver(solver, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped ising;
  Tabu<::model::IsingGrouped> solver;
  auto result = run_solver(solver, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  Tabu<::model::PuboWithCounter<uint64_t>> solver;
  auto result = run_solver(solver, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  Tabu<::model::IsingTermCached> solver;
  auto result = run_solver(solver, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, PuboEmptyMixedModel)
{
  std::set<int> disabled = {2};
  utils::set_disabled_feature(disabled);
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  Tabu<::model::PuboWithCounter<uint64_t>> solver;
  auto result = run_solver(solver, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(Tabu, IsingEmptyMixedModel)
{
  std::set<int> disabled = {2};
  utils::set_disabled_feature(disabled);
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  Tabu<::model::IsingTermCached> solver;
  auto result = run_solver(solver, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(Tabu, ConstPuboGroupedModel)
{
  std::string input_file(utils::data_path("constpubogrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> model;
  Tabu<::model::PuboGrouped<uint64_t>> solver;
  auto result = run_solver(solver, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(Tabu, ConstIsingGroupedModel)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped model;
  Tabu<::model::IsingGrouped> solver;
  auto result = run_solver(solver, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}