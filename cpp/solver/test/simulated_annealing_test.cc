
#include "solver/simulated_annealing.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "utils/random_generator.h"
#include "gtest/gtest.h"
#include "markov/model.h"
#include "model/ising.h"
#include "model/ising_grouped.h"
#include "model/pubo.h"
#include "model/pubo_grouped.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::solver::SimulatedAnnealing;

class SimulatedAnnealingTest : public ::testing::Test
{
 public:
  void configure(const std::string& params_data)
  {
    auto input = utils::json_from_string(R"({
      "cost_function": {
        "version": "1.0",
        "type": "toy"
      }
    })");
    toy_.configure(input);
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

  std::string AsString(const utils::Structure& structure)
  {
    std::string rendered = structure.to_string();
#ifdef __INTEL_COMPILER
    // gtest with Intel compiler has a bug where it doesn't parse raw
    // strings literals properly in EXPECT_EQ(...):
    // Lines are separated by \\n // instead of \n.
    size_t pos;
    while ((pos = rendered.find("\n")) != std::string::npos)
    {
      rendered.replace(pos, 1, "\\n");
    }
#endif
    return rendered;
  }

 protected:
  TestModel toy_;
  SimulatedAnnealing<TestModel> solver_;
};

TEST_F(SimulatedAnnealingTest, SimulatesToyModel)
{
  auto result = run(R"({
    "params": {
      "step_limit": 100,
      "seed": 42,
      "restarts": 4,
      "schedule": {
        "type": "linear",
        "initial": 2.0,
        "final": 1.0
      },
      "number_of_solutions": 2
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_EQ(result["solutions"]["parameters"]["restarts"].get<size_t>(), 4);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["schedule"]["initial"].get<double>(),
      2.0);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["schedule"]["final"].get<double>(),
      1.0);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);
}

TEST_F(SimulatedAnnealingTest, SimulatesToyModelGeometric)
{
  auto result = run(R"({
    "params": {
      "step_limit": 100,
      "seed": 42,
      "restarts": 4,
      "schedule": {
        "type": "geometric",
        "initial": 10,
        "final": 1,
        "start": 0,
        "stop": 99
      },
      "number_of_solutions": 2
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_EQ(result["solutions"]["parameters"]["restarts"].get<size_t>(), 4);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["schedule"]["initial"].get<double>(),
      10.0);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["schedule"]["final"].get<double>(),
      1.0);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);
}

TEST_F(SimulatedAnnealingTest, SimulatesDefaultParameters)
{
  auto result = run(R"({
    "params": {
      "step_limit": 100,
      "seed": 42,
      "restarts": 4
    }
  })");
  EXPECT_TRUE(solver_.use_inverse_temperature());
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
}

TEST_F(SimulatedAnnealingTest, SimulatesToyModelWithBetaStartStop)
{
  auto result = run(R"({
    "params": {
      "step_limit": 100,
      "restarts": 4,
      "seed": 42,
      "beta_start": 0.5,
      "cost_limit": 0,
      "beta_stop": 10.0
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);

  auto solver = result["benchmark"].get_extension("solver");
  std::cout << solver.to_string() << std::endl;
#if defined(qiotoolkit_PROFILING) || defined(_DEBUG)
  EXPECT_EQ(AsString(solver), R"({
  "cost_limit": 0.000000,
  "cost_milestones": [
    {"cost": 9.464013, "step": 1.000000},
    {"cost": 0.794104, "step": 2.000000},
    {"cost": 0.245903, "step": 3.000000},
    {"cost": 0.132852, "step": 4.000000},
    {"cost": 0.097936, "step": 6.000000},
    {"cost": 0.000760, "step": 7.000000},
    {"cost": 0.000191, "step": 84.000000}
  ],
  "exit_reason": "Stop due to step limit: 100, steps:100",
  "last_step": 100,
  "step_limit": 100
})");
#endif
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(),
            1);  // default array size
}

TEST_F(SimulatedAnnealingTest, SimulatesInvalidMultipleSolutionValue)
{
  auto params_data = R"({
    "params": {
      "seed": 42,
      "restarts": 4,
      "number_of_solutions": 3000000
    }
  })";
  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST(SimulatedAnnealing, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  SimulatedAnnealing<::model::PuboWithCounter<uint64_t>> sa;
  auto result = run_solver(sa, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-12, result["solutions"]["cost"].get<double>());
}

TEST(SimulatedAnnealing, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  SimulatedAnnealing<::model::PuboWithCounter<uint64_t>> sa;
  auto result = run_solver(sa, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(SimulatedAnnealing, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  SimulatedAnnealing<::model::IsingTermCached> sa;
  auto result = run_solver(sa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(SimulatedAnnealing, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  SimulatedAnnealing<::model::PuboWithCounter<uint64_t>> sa;
  auto result = run_solver(sa, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(SimulatedAnnealing, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> pubo;
  SimulatedAnnealing<::model::PuboGrouped<uint64_t>> sa;
  auto result = run_solver(sa, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(SimulatedAnnealing, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  SimulatedAnnealing<::model::IsingTermCached> sa;
  auto result = run_solver(sa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(SimulatedAnnealing, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped ising;
  SimulatedAnnealing<::model::IsingGrouped> sa;
  auto result = run_solver(sa, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

TEST(SimulatedAnnealing, ConstPuboGroupedModel)
{
  std::string input_file(utils::data_path("constpubogrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> model;
  SimulatedAnnealing<::model::PuboGrouped<uint64_t>> sa;
  auto result = run_solver(sa, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(SimulatedAnnealing, ConstIsingGroupedModel)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped model;
  SimulatedAnnealing<::model::IsingGrouped> sa;
  auto result = run_solver(sa, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(SimulatedAnnealing, InvalidSweepsOne)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params.sweeps1.json"));
  ::model::IsingGrouped model;
  SimulatedAnnealing<::model::IsingGrouped> sa;
  EXPECT_THROW(run_solver(sa, model, input_file, param_file),
               utils::ValueException);
}