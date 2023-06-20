// Copyright (c) Microsoft. All Rights Reserved.
#include "solver/parallel_tempering.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "utils/random_generator.h"
#include "gtest/gtest.h"
#include "model/ising.h"
#include "model/ising_grouped.h"
#include "model/pubo.h"
#include "model/pubo_grouped.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::Structure;
using ::solver::ParallelTempering;

class ParallelTemperingTest : public ::testing::Test
{
 public:
  void configure(const std::string& params_data)
  {
    std::string input(R"(
    {
      "cost_function": 
      {
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
  ParallelTempering<TestModel> solver_;
};

TEST_F(ParallelTemperingTest, SimulatesToyModel)
{
  auto result = run(R"({
    "params": {
      "step_limit": 200,
      "temperatures": [0.1, 0.2, 0.3, 0.4],
      "threads": 1,
      "number_of_solutions": 2
    }
  })");
  std::cout << result << std::endl;
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
  EXPECT_EQ(result["solutions"]["parameters"]["step_limit"].get<uint64_t>(),
            200);
  EXPECT_DOUBLE_EQ(
      result["solutions"]["parameters"]["temperatures"]["segments"][0]["value"]
          .get<double>(),
      0.1);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);
}

TEST_F(ParallelTemperingTest, SimulatesToyModelInParallel)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 100,
      "temperatures": [0.1, 0.2, 0.3, 0.4]
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
  EXPECT_EQ(result["solutions"]["parameters"]["seed"].get<uint32_t>(), 42);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(),
            1);  // default array size
}

TEST_F(ParallelTemperingTest, SimulatesToyModelWithLinearTemperatureSet)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 200,
      "cost_limit": 0,
      "temperatures": {
        "type": "linear",
        "initial": 0.5,
        "final": 0.1,
        "count": 5
      }
    },
    "observer": {
      "group_by": ["replica"],
      "constant": ["T"],
      "average": ["avg_cost", "acc_rate", "avg_dir", "swap_rate"]
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);

  auto solver = result["benchmark"].get_extension("solver");
  EXPECT_EQ(solver["last_step"].get<uint64_t>(), 200);
  // NOTE: The following tests depend on internal implementation details and
  // are prone to change (their purpose is to highlight the output format).
  std::cout << solver["cost_milestones"].to_string() << std::endl;
#if defined(qiotoolkit_PROFILING) || defined(_DEBUG)
  EXPECT_EQ(AsString(solver["cost_milestones"]), R"([
  {"cost": 9.464013, "step": 1.000000},
  {"cost": 0.145333, "step": 2.000000},
  {"cost": 0.076090, "step": 3.000000},
  {"cost": 0.020307, "step": 5.000000},
  {"cost": 0.003722, "step": 12.000000},
  {"cost": 0.000020, "step": 88.000000}
])");
#endif

  std::cout << solver["replica"].to_string() << std::endl;
  EXPECT_EQ(AsString(solver["replica"]), R"([
  {
    "T": 0.100000,
    "acc_rate": 0.053000,
    "avg_cost": 0.334950,
    "avg_dir": 0.000000,
    "swap_rate": 0.630000
  },
  {
    "T": 0.200000,
    "acc_rate": 0.076500,
    "avg_cost": 0.448586,
    "avg_dir": 0.515000,
    "swap_rate": 0.850000
  },
  {
    "T": 0.300000,
    "acc_rate": 0.110000,
    "avg_cost": 0.426404,
    "avg_dir": 0.565000,
    "swap_rate": 0.800000
  },
  {
    "T": 0.400000,
    "acc_rate": 0.159000,
    "avg_cost": 0.460988,
    "avg_dir": 0.705000,
    "swap_rate": 0.880000
  },
  {
    "T": 0.500000,
    "acc_rate": 0.208000,
    "avg_cost": 0.596713,
    "avg_dir": 0.995000
  }
])");
}

TEST_F(ParallelTemperingTest, VerifiesTemperaturesDefault)
{
  configure(R"({
    "params": {
      "step_limit": 100,
      "seed": 42
    }
  })");
  std::vector<double> temps = solver_.get_temperatures();
  EXPECT_TRUE(solver_.use_inverse_temperatures());
  EXPECT_EQ(0.1, temps[0]);
  EXPECT_EQ(0.5, temps[1]);
  EXPECT_EQ(1., temps[2]);
  EXPECT_EQ(2., temps[3]);
  EXPECT_EQ(4., temps[4]);
}

TEST_F(ParallelTemperingTest, SimulatesInvalidMultipleSolutionValue)
{
  auto params_data = R"({
    "params": {
      "step_limit": 100,
      "seed": 42,
      "number_of_solutions": 3000000
    }
  })";
  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST_F(ParallelTemperingTest, VerifiesTemperaturesValid)
{
  auto params_data = R"({
    "params": {
      "step_limit": 100,
      "temperatures": [],
      "seed": 42
    }
  })";
  EXPECT_THROW_MESSAGE(
      configure(params_data), utils::ConfigurationException,
      std::string("parameter `temperatures`: unable to initialize schedule:") +
          "\n  Linear: parameter `type` is required" +
          "\n  Geometric: parameter `type` is required" +
          "\n  Segments: `segments` must be specified as a non-empty array." +
          "\n  Constant: expected double, found array");
}

TEST(ParallelTempering, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  ParallelTempering<::model::PuboWithCounter<uint64_t>> pt;
  auto result = run_solver(pt, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(ParallelTempering, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  ParallelTempering<::model::IsingTermCached> pt;
  auto result = run_solver(pt, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(ParallelTempering, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  ParallelTempering<::model::PuboWithCounter<uint64_t>> pt;
  auto result = run_solver(pt, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(ParallelTempering, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingTermCached ising;
  ParallelTempering<model::IsingTermCached> pt;
  auto result = run_solver(pt, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(ParallelTempering, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> pubo;
  ParallelTempering<::model::PuboGrouped<uint64_t>> pt;
  auto result = run_solver(pt, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(ParallelTempering, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped ising;
  ParallelTempering<::model::IsingGrouped> pt;
  auto result = run_solver(pt, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

TEST(ParallelTempering, ConstPuboGroupedModel)
{
  std::string input_file(utils::data_path("constpubogrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::PuboGrouped<uint64_t> model;
  ParallelTempering<::model::PuboGrouped<uint64_t>> pt;
  auto result = run_solver(pt, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(ParallelTempering, ConstIsingGroupedModel)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params1.json"));
  ::model::IsingGrouped model;
  ParallelTempering<::model::IsingGrouped> pt;
  auto result = run_solver(pt, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}