

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
#include "gtest/gtest.h"
#include "solver/test/test_model.h"
#include "solver/test/test_utils.h"
#include "solver/quantum_monte_carlo.h"

using ::utils::Json;
using ::solver::QuantumMonteCarlo;

class QuantumMonteCarloTest : public ::testing::Test
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
  QuantumMonteCarlo<TestModel> solver_;
};

TEST_F(QuantumMonteCarloTest, SimulatesToyModel)
{
  auto result = run(R"({
    "params": {
      "seed": 42,
      "step_limit": 1000,
      "beta_start": 0.1,
      "beta_stop": 10,
      "trotter_number": 16,
      "number_of_solutions": 2,
      "restarts": 8
    }
  })");
  EXPECT_LT(std::abs(result["solutions"]["cost"].get<double>()), 1);
  EXPECT_LT(std::abs(result["solutions"]["configuration"].get<double>() - 42),
            1);
  EXPECT_EQ(result["solutions"]["solutions"].get_array_size(), 2);

}

TEST_F(QuantumMonteCarloTest, SimulatesInvalidMultipleSolutionValue)
{
  auto params_data = R"({
    "params": {
      "step_limit": 100,
      "step_limit": 1000,
      "number_of_solutions": 3000000
    }
  })";
  EXPECT_THROW(configure(params_data), utils::ValueException);
}

TEST(QuantumMonteCarlo, PuboModel)
{
  std::string input_file(utils::data_path("cpupubojobstest.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  QuantumMonteCarlo<model::PuboWithCounter<uint64_t>> qmc;
  auto result = run_solver(qmc, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-12, result["solutions"]["cost"].get<double>());
}

TEST(QuantumMonteCarlo, PuboEmptyModel)
{
  std::string input_file(utils::data_path("puboempty.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  QuantumMonteCarlo<model::PuboWithCounter<uint64_t>> qmc;
  auto result = run_solver(qmc, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(2, result["solutions"]["cost"].get<double>());
}

TEST(QuantumMonteCarlo, IsingEmptyModel)
{
  std::string input_file(utils::data_path("isingempty.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::IsingTermCached ising;
  QuantumMonteCarlo<::model::IsingTermCached> qmc;
  auto result = run_solver(qmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(12, result["solutions"]["cost"].get<double>());
}

TEST(QuantumMonteCarlo, IsingModel)
{
  std::string input_file(utils::data_path("ising1.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::IsingTermCached ising;
  QuantumMonteCarlo<::model::IsingTermCached> qmc;
  auto result = run_solver(qmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
}

TEST(QuantumMonteCarlo, PuboEmptyMixedModel)
{
  std::string input_file(utils::data_path("puboemptymixed.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::PuboWithCounter<uint64_t> pubo;
  QuantumMonteCarlo<model::PuboWithCounter<uint64_t>> qmc;
  auto result = run_solver(qmc, pubo, input_file, param_file);
  EXPECT_DOUBLE_EQ(-104, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["12"].get<int>());
}

TEST(QuantumMonteCarlo, IsingEmptyMixedModel)
{
  std::string input_file(utils::data_path("isingemptymixed.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::IsingTermCached ising;
  QuantumMonteCarlo<::model::IsingTermCached> qmc;
  auto result = run_solver(qmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-100, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["10"].get<int>());
}

TEST(QuantumMonteCarlo, PuboGroupedModel)
{
  std::string input_file(utils::data_path("cpupubogroupedjobstest.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::PuboGrouped<uint64_t> pubo;
  QuantumMonteCarlo<::model::PuboGrouped<uint64_t>> qmc;
  auto result = run_solver(qmc, pubo, input_file, param_file);
  EXPECT_LE(-12, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["1"].get<int>());
  EXPECT_EQ(0, result["solutions"]["configuration"]["3"].get<int>());
  EXPECT_EQ(1, result["solutions"]["configuration"]["4"].get<int>());
}

TEST(QuantumMonteCarlo, IsingGroupedModel)
{
  std::string input_file(utils::data_path("isinggrouped1.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::IsingGrouped ising;
  QuantumMonteCarlo<::model::IsingGrouped> qmc;
  auto result = run_solver(qmc, ising, input_file, param_file);
  EXPECT_DOUBLE_EQ(-10, result["solutions"]["cost"].get<double>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["0"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["2"].get<int>());
  EXPECT_EQ(-1, result["solutions"]["configuration"]["3"].get<int>());
}

TEST(QuantumMonteCarlo, ConstPuboGroupedModel)
{
  std::string input_file(utils::data_path("constpubogrouped.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::PuboGrouped<uint64_t> model;
  QuantumMonteCarlo<::model::PuboGrouped<uint64_t>> qmc;
  auto result = run_solver(qmc, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

TEST(QuantumMonteCarlo, ConstIsingGroupedModel)
{
  std::string input_file(utils::data_path("constisinggrouped.json"));
  std::string param_file(utils::data_path("params_qmc.json"));
  ::model::IsingGrouped model;
  QuantumMonteCarlo<::model::IsingGrouped> ssmc;
  auto result = run_solver(ssmc, model, input_file, param_file);
  EXPECT_EQ(62, result["solutions"]["cost"].get<double>());
}

/// Test for code pattern used for parallel computing of nested loops.
/// Used summation += operator to test absence of unwanted repetitions.
/// Used cross indexing to test correct synchronization.
TEST(QuantumMonteCarlo, OmpCodePattern)
{
  size_t sweep_size = 100;
  size_t total_states = 16;

  std::vector<std::vector<size_t>> 
      clusters(sweep_size),
      expected_clusters(sweep_size);

  for (size_t var_index = 0; var_index < sweep_size; var_index++)
  {
    clusters[var_index].resize(total_states, 0);
    expected_clusters[var_index].resize(total_states, 0);
  }

  // Non parallel version of the code
  for (size_t var_index = 0; var_index < sweep_size; var_index++)
  {
    for (size_t i = 0; i < total_states; i++)
    {
      expected_clusters[var_index][i] += i;
    }

    for (size_t i = 0; i < total_states; i++)
    {
      expected_clusters[var_index][total_states - i - 1] += i;
    }
  }
  //----------

  // Parallel version of the code
  #pragma omp parallel
  {
    for (size_t var_index = 0; var_index < sweep_size; var_index++)
    {
      #pragma omp for
      for (size_t i = 0; i < total_states; i++)
      {
        clusters[var_index][i] += i;
      }

      #pragma omp for
      for (size_t i = 0; i < total_states; i++)
      {
        clusters[var_index][total_states - i - 1] += i;
      }
    }
  }
  //----------

  EXPECT_EQ(expected_clusters, clusters);
}