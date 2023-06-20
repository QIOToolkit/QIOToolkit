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
#include "solver/pa_parameter_free.h"
#include "solver/parameter_free_solver.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::solver::PAParameterFree;
using ::solver::ParameterFreeSolver;

TEST(PAParameterFree, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      PAParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          PAParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_NEAR(-12, result["solutions"]["cost"].get<double>(), 0.000001);
}

TEST(PAParameterFree, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      PAParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          PAParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(PAParameterFree, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingTermCached,
                      PAParameterFree<model::IsingTermCached>>
      pa_pf;
  model::IsingTermCached model;
  auto result =
      run_solver<ParameterFreeSolver<model::IsingTermCached,
                                     PAParameterFree<model::IsingTermCached>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(PAParameterFree, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingTermCached,
                      PAParameterFree<model::IsingTermCached>>
      pa_pf;
  model::IsingTermCached model;
  auto result =
      run_solver<ParameterFreeSolver<model::IsingTermCached,
                                     PAParameterFree<model::IsingTermCached>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_NEAR(-10, result["solutions"]["cost"].get<double>(), 0.000001);
}

TEST(PAParameterFree, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      PAParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          PAParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_NEAR(-104, result["solutions"]["cost"].get<double>(), 0.000001);
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(PAParameterFree, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingTermCached,
                      PAParameterFree<model::IsingTermCached>>
      pa_pf;
  model::IsingTermCached model;
  auto result =
      run_solver<ParameterFreeSolver<model::IsingTermCached,
                                     PAParameterFree<model::IsingTermCached>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(PAParameterFree, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::PuboGrouped<uint64_t>,
                      PAParameterFree<model::PuboGrouped<uint64_t>>>
      pa_pf;
  model::PuboGrouped<uint64_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboGrouped<uint64_t>,
                          PAParameterFree<model::PuboGrouped<uint64_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(PAParameterFree, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeSolver<model::IsingGrouped, PAParameterFree<model::IsingGrouped>>
      pa_pf;
  model::IsingGrouped model;
  auto result =
      run_solver<ParameterFreeSolver<model::IsingGrouped,
                                     PAParameterFree<model::IsingGrouped>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

#ifndef _DEBUG
TEST(PAParameterFree, HonoringTimeout)
{
  std::string input_file(utils::data_path("mixedconstants.json"));
  std::string param_file(utils::data_path("pf10s.json"));
  ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                      PAParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  auto start_time = std::chrono::high_resolution_clock::now();
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeSolver<model::PuboWithCounter<uint8_t>,
                          PAParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  auto end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_learning =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();

  EXPECT_LT(8000, ms_diff_learning);
  EXPECT_LT(ms_diff_learning, 12000);
}
#endif
