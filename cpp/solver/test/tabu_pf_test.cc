
#include <cmath>
#include <sstream>
#include <string>

#include "utils/file.h"
#include "utils/json.h"
#include "utils/random_generator.h"
#include "model/ising.h"
#include "model/ising_grouped.h"
#include "model/pubo.h"
#include "model/pubo_grouped.h"
#include "solver/tabu_parameter_free.h"
#include "solver/parameter_free_linear_solver.h"
#include "gtest/gtest.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"

using ::utils::Json;
using ::solver::TabuParameterFree;
using ::solver::ParameterFreeLinearSearchSolver;

TEST(TabuParameterFree, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                      TabuParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                          TabuParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_NEAR(-12, result["solutions"]["cost"].get<double>(), 0.000001);
}

TEST(TabuParameterFree, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                      TabuParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                          TabuParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(TabuParameterFree, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::IsingTermCached,
                      TabuParameterFree<model::IsingTermCached>>
      pa_pf;
  model::IsingTermCached model;
  auto result =
      run_solver<ParameterFreeLinearSearchSolver<model::IsingTermCached,
                                     TabuParameterFree<model::IsingTermCached>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(TabuParameterFree, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::IsingTermCached,
                      TabuParameterFree<model::IsingTermCached>>
      pa_pf;
  model::IsingTermCached model;
  auto result =
      run_solver<ParameterFreeLinearSearchSolver<model::IsingTermCached,
                                     TabuParameterFree<model::IsingTermCached>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_NEAR(-10, result["solutions"]["cost"].get<double>(), 0.000001);
}

TEST(TabuParameterFree, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                      TabuParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                          TabuParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_NEAR(-104, result["solutions"]["cost"].get<double>(), 0.000001);
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(TabuParameterFree, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::IsingTermCached,
                      TabuParameterFree<model::IsingTermCached>>
      pa_pf;
  model::IsingTermCached model;
  auto result =
      run_solver<ParameterFreeLinearSearchSolver<model::IsingTermCached,
                                     TabuParameterFree<model::IsingTermCached>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(TabuParameterFree, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::PuboGrouped<uint64_t>,
                      TabuParameterFree<model::PuboGrouped<uint64_t>>>
      pa_pf;
  model::PuboGrouped<uint64_t> model;
  auto result = run_solver<
      ParameterFreeLinearSearchSolver<model::PuboGrouped<uint64_t>,
                          TabuParameterFree<model::PuboGrouped<uint64_t>>>>(
      pa_pf, model, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(TabuParameterFree, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("pf5s.json"));
  ParameterFreeLinearSearchSolver<model::IsingGrouped, TabuParameterFree<model::IsingGrouped>>
      pa_pf;
  model::IsingGrouped model;
  auto result =
      run_solver<ParameterFreeLinearSearchSolver<model::IsingGrouped,
                                     TabuParameterFree<model::IsingGrouped>>>(
          pa_pf, model, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

#ifndef _DEBUG
TEST(TabuParameterFree, HonoringTimeout)
{
  std::string input_file(utils::data_path("mixedconstants.json"));
  std::string param_file(utils::data_path("pf10s.json"));
  ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                      TabuParameterFree<model::PuboWithCounter<uint8_t>>>
      pa_pf;
  auto start_time = std::chrono::high_resolution_clock::now();
  model::PuboWithCounter<uint8_t> model;
  auto result = run_solver<
      ParameterFreeLinearSearchSolver<model::PuboWithCounter<uint8_t>,
                          TabuParameterFree<model::PuboWithCounter<uint8_t>>>>(
      pa_pf, model, input_file, param_file);
  auto end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_learning =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();
  //To do: choose different dataset
  EXPECT_LT(100, ms_diff_learning);
  EXPECT_LT(ms_diff_learning, 12000);
}
#endif
