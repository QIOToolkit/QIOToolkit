// Copyright (c) Microsoft. All Rights Reserved.
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
#include "solver/parameter_free_solver.h"
#include "solver/ssmc_parameter_free.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::solver::ParameterFreeSolver;
using ::solver::SSMCParameterFree;

TEST(SSMCParameterFree, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      SSMCParameterFree<model::PuboWithCounter<uint8_t>>>
      ssmc_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          SSMCParameterFree<model::PuboWithCounter<uint8_t>>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-12, result["solutions"]["cost"].get<double>());
}

TEST(SSMCParameterFree, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      SSMCParameterFree<model::PuboWithCounter<uint8_t>>>
      ssmc_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          SSMCParameterFree<model::PuboWithCounter<uint8_t>>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(SSMCParameterFree, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingTermCached,
                      SSMCParameterFree<model::IsingTermCached>>
      ssmc_pf;
  model::IsingTermCached model;
  auto result = run_solver<ParameterFreeSolver<
      model::IsingTermCached, SSMCParameterFree<model::IsingTermCached>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(SSMCParameterFree, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingTermCached,
                      SSMCParameterFree<model::IsingTermCached>>
      ssmc_pf;
  model::IsingTermCached model;
  auto result = run_solver<ParameterFreeSolver<
      model::IsingTermCached, SSMCParameterFree<model::IsingTermCached>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(SSMCParameterFree, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      SSMCParameterFree<model::PuboWithCounter<uint8_t>>>
      ssmc_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          SSMCParameterFree<model::PuboWithCounter<uint8_t>>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(SSMCParameterFree, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingTermCached,
                      SSMCParameterFree<model::IsingTermCached>>
      ssmc_pf;
  model::IsingTermCached model;
  auto result = run_solver<ParameterFreeSolver<
      model::IsingTermCached, SSMCParameterFree<model::IsingTermCached>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(SSMCParameterFree, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboGrouped<uint64_t>,
                      SSMCParameterFree<model::PuboGrouped<uint64_t>>>
      ssmc_pf;
  model::PuboGrouped<uint64_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboGrouped<uint64_t>,
                          SSMCParameterFree<model::PuboGrouped<uint64_t>>>>(
      ssmc_pf, model, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(SSMCParameterFree, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingGrouped,
                      SSMCParameterFree<model::IsingGrouped>>
      ssmc_pf;
  model::IsingGrouped model;
  auto result =
      run_solver<ParameterFreeSolver<model::IsingGrouped,
                                     SSMCParameterFree<model::IsingGrouped>>>(
          ssmc_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

#ifndef _DEBUG
TEST(SSMCParameterFree, HonoringTimeout)
{
  std::string input_file(utils::data_path("mixedconstants.json"));
  std::string param_file(utils::data_path("pf10s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      SSMCParameterFree<model::PuboWithCounter<uint8_t>>>
      ssmc_pf;
  auto start_time = std::chrono::high_resolution_clock::now();
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          SSMCParameterFree<model::PuboWithCounter<uint8_t>>>>(
      ssmc_pf, model, input_file, param_file);
  auto end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_learning =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();

  // Modified to throw only a warning when faster or at most 30% slower.
  // This helps reduce the flakiness of this test (and allows us to actually
  // turn on the release mode test pipeline).
  if (ms_diff_learning < 9000)
  {
    LOG(WARN,
        "More than 10% faster than expecter timeout (10s): ", ms_diff_learning,
        "ms");
  }
  if (ms_diff_learning > 13000)
  {
    LOG(ERROR,
        "More than 30% slower than expecter timeout (10s): ", ms_diff_learning,
        "ms");
  }
  if (ms_diff_learning > 11000)
  {
    // Only warn between 11s and 13s.
    LOG(WARN,
        "More than 10% slower than expecter timeout (10s): ", ms_diff_learning,
        "ms");
  }
  // Fail for more than 13s.
  EXPECT_LE(ms_diff_learning, 13000);
}
#endif
