// Copyright (c) Microsoft. All Rights Reserved.
#include "../ising.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/file.h"
#include "../../utils/proto_reader.h"
#include "../../utils/stream_handler_json.h"
#include "../../utils/stream_handler_proto.h"
#include "../../solver/all_solvers.h"
#include "gtest/gtest.h"
#include "problem.pb.h"
#include "test_utils.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::Twister;
using ::model::Ising;
using ::model::IsingState;
using ::model::IsingTermCached;
using ::model::IsingTermCachedState;
using IsingCompact = ::model::IsingCompact<uint8_t>;
using IsingCompact16 = ::model::IsingCompact<uint16_t>;
using IsingCompact32 = ::model::IsingCompact<uint32_t>;
using ::model::IsingCompactState;
using ::solver::create_solver;

class IsingTest : public testing::Test
{
 public:
  IsingTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input_str(R"({
      "cost_function": {
        "type": "ising",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [0, 1]},
          {"c": 1, "ids": [1, 2]},
          {"c": 1, "ids": [2, 3]},
          {"c": 1, "ids": [3, 4]},
          {"c": 1, "ids": [4, 5]},
          {"c": 1, "ids": [5, 6]},
          {"c": 1, "ids": [6, 7]},
          {"c": 1, "ids": [7, 8]},
          {"c": 1, "ids": [8, 9]},
          {"c": 1, "ids": [9, 0]}
        ],
        "initial_configuration": {
          "0": -1, "1": 1, "2": 1, "3": 1,  "4": 1,
          "5": -1, "6": -1, "7": 1, "8": 1, "9": -1
        }
      }
    })");
    utils::configure_with_configuration_from_json_string(input_str, ising);

    create_proto_test_problem(AzureQuantum::Problem_ProblemType_ISING);
    utils::configure_with_configuration_from_proto_folder(
        utils::data_path("models_input_problem_pb"), ising_proto);
  }

  Ising ising;
  Ising ising_proto;
};

TEST_F(IsingTest, InitialConfiguration)
{
  IsingState state = ising.get_initial_configuration_state();
  std::vector<bool> expected_spins = {true, false, false, false, false,
                                      true, true,  false, false, true};
  EXPECT_EQ(state.spins, expected_spins);
}

TEST(Ising, InitialConfigurationWrongValue)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}],
      "initial_configuration": {
          "0": 0, "1": 1
      }
    }
  })";
  Ising ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  EXPECT_THROW_MESSAGE(ising.get_initial_configuration_state(),
                       utils::ValueException,
                       "parameter `initial_configuration` may have 1 and -1 "
                       "values for ising models, found: 0");
}

TEST(Ising, InitialConfigurationWrongNode)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}],
      "initial_configuration": {
          "0": -1, "2": 1
      }
    }
  })";
  Ising ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, ising),
      utils::ValueException,
      "parameter `initial_configuration` have node "
      "name: 2 which is not found in model node names");
}

TEST_F(IsingTest, SingleUpdates)
{
  IsingState state(10, 10);
  EXPECT_EQ(ising.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(ising.calculate_cost_difference(state, transition), -4);

  ising.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 6);

  EXPECT_EQ(ising.calculate_cost_difference(state, transition), 4);

  ising.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 10);
}

TEST_F(IsingTest, Metropolis)
{
  IsingState state(10, 10);

  EXPECT_EQ(ising.calculate_cost(state), 10);  // Maximum value

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 10;
  double cost = 10;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = ising.get_random_transition(state, rng);
    auto diff = ising.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      ising.apply_transition(transition, state);
      cost += diff;
    }
    min = std::min(min, cost);
  }
  EXPECT_EQ(min, -10);
}

TEST_F(IsingTest, MinCostMinimum)
{
  double min_cost = ising.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ((MIN_DELTA_DEFAULT)*2, min_cost);
  double max_cost = ising.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(4, max_cost);
}

TEST_F(IsingTest, SingleUpdates_proto)
{
  IsingState state(10, 10);
  EXPECT_EQ(ising_proto.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(ising_proto.calculate_cost_difference(state, transition), -4);

  ising_proto.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising_proto.calculate_cost(state), 6);

  EXPECT_EQ(ising_proto.calculate_cost_difference(state, transition), 4);

  ising_proto.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising_proto.calculate_cost(state), 10);
}

TEST_F(IsingTest, Metropolis_proto)
{
  IsingState state(10, 10);

  EXPECT_EQ(ising_proto.calculate_cost(state), 10);  // Maximum value

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 10;
  double cost = 10;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = ising_proto.get_random_transition(state, rng);
    auto diff = ising_proto.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      ising_proto.apply_transition(transition, state);
      cost += diff;
    }
    min = std::min(min, cost);
  }
  EXPECT_EQ(min, -10);
}

TEST_F(IsingTest, MinCostMinimum_proto)
{
  double min_cost = ising_proto.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ((MIN_DELTA_DEFAULT)*2, min_cost);
  double max_cost = ising_proto.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(4, max_cost);
}

TEST(Ising, VerifiesTypeLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Ising ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, ising),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(Ising, VerifiesTypeLabelValue)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "foo",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Ising ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, ising),
      ConfigurationException,
      "parameter `type`: must be equal to 'ising', found 'foo'");
}

TEST(Ising, VerifiesVersionLabelPresent)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Ising ising;

  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, ising),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(Ising, VerifiesVersionLabelValue)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "-7",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Ising ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, ising),
      ConfigurationException,
      "Expecting `version` equals 1.0 or 1.1, but found: -7");
}

TEST(Ising, DisorderedIDsInTerm)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.1",
      "terms": [{"c":1, "ids":[-1,2]}, {"c":2, "ids":[2,100]}]
    }
  })";
  Ising ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  IsingState state(3, 0);
  state.spins[0] = false;
  state.spins[1] = true;
  state.spins[2] = false;
  auto result = ising.render_state(state);
  EXPECT_EQ(1, result["-1"].get<int32_t>());
  EXPECT_EQ(-1, result["2"].get<int32_t>());
  EXPECT_EQ(1, result["100"].get<int32_t>());
}

TEST(IsingCached, DisorderedIDsInTerm)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.1",
      "terms": [{"c":1, "ids":[0,-6]}, {"c":2, "ids":[-6,100]}]
    }
  })";
  IsingTermCached ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  IsingTermCachedState state(3, 0);
  state.spins[0] = true;
  state.spins[1] = true;
  state.spins[2] = false;
  auto result = ising.render_state(state);
  EXPECT_EQ(-1, result["0"].get<int32_t>());
  EXPECT_EQ(-1, result["-6"].get<int32_t>());
  EXPECT_EQ(1, result["100"].get<int32_t>());
}

TEST(IsingCached, ConstantTerms)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.1",
      "terms": [{"c":100, "ids":[]}, {"c":2, "ids":[]}]
    }
  })";
  IsingTermCached ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  IsingTermCachedState state(0, 2);
  auto cost = ising.calculate_cost(state) + ising.get_const_cost();
  EXPECT_EQ(102, cost);
  auto result = ising.render_state(state);
  EXPECT_EQ(std::string("{}"), result.to_string());
}

TEST(Ising, ConstantTerms)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.1",
      "terms": [{"c":100, "ids":[]}]
    }
  })";
  Ising ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  IsingState state(0, 1);
  auto cost = ising.calculate_cost(state) + ising.get_const_cost();
  EXPECT_EQ(100, cost);
  auto result = ising.render_state(state);
  EXPECT_EQ(std::string("{}"), result.to_string());
}

TEST(Ising, DuplicatedIDsInTerm)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [{"c":1, "ids":[-1,2,2]}]
    }
  })");

  Ising ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, ising),
      utils::DuplicatedVariableException, "Duplicated ids detected in term!");
}

TEST(Ising, StateVectorMemoryEstimate)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]},
        {"c": 1, "ids": [2, 3]},
        {"c": 1, "ids": [3, 4]},
        {"c": 1, "ids": [4, 5]},
        {"c": 1, "ids": [5, 6]},
        {"c": 1, "ids": [6, 7]},
        {"c": 1, "ids": [7, 8]},
        {"c": 1, "ids": [8, 9]},
        {"c": 1, "ids": [9, 0]}
      ]
    }
  })";
  IsingTermCached ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);

  size_t state_mem = ising.state_memory_estimate();
  // State contains total 2 small bool vectors, fitting in 4 bytes each
  size_t expected_state_mem = sizeof(IsingTermCached::State_T) + 4 + 4;
  EXPECT_EQ(state_mem, expected_state_mem);
}

TEST(Ising, MinCost)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": -2, "ids": [1, 2]},
        {"c": 3, "ids": [2, 3]},
        {"c": -4, "ids": [3, 4]},
        {"c": 5, "ids": [4, 5]},
        {"c": -6, "ids": [5, 6]},
        {"c": 7, "ids": [6, 7]},
        {"c": -8, "ids": [7, 8]},
        {"c": 9, "ids": [8, 9]},
        {"c": -9.5, "ids": [9, 0]}
      ]
    }
  })");
  IsingTermCached ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);

  double min_cost = ising.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}

TEST(Ising, MinCost_proto)
{
  IsingTermCached ising_proto;

  create_proto_test_problem2(AzureQuantum::Problem_ProblemType_ISING);
  utils::configure_with_configuration_from_proto_folder(
      utils::data_path("models_input_problem2_pb"), ising_proto);
  double min_cost = ising_proto.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}

TEST(IsingCompact, MinCost)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": -2, "ids": [1, 2]},
        {"c": 3, "ids": [2, 3]},
        {"c": -4, "ids": [3, 4]},
        {"c": 5, "ids": [4, 5]},
        {"c": -6, "ids": [5, 6]},
        {"c": 7, "ids": [6, 7]},
        {"c": -8, "ids": [7, 8]},
        {"c": 9, "ids": [8, 9]},
        {"c": -9.5, "ids": [9, 0]}
      ]
    }
  })");
  IsingCompact ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  ising.init();

  double min_cost = ising.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}

TEST(IsingCompact, DuplicatedIDsInTerm)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "ising",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,-1]}, {"c":8, "ids":[0,-100]}, {"c":10, "ids":[-1,2,-1]}]
    }
  })");

  IsingCompact ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  EXPECT_THROW_MESSAGE(ising.init(), utils::DuplicatedVariableException,
                       "Duplicated ids detected in term!");
}

TEST(IsingCompact, ConstantTerms)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "ising",
      "version": "1.1",
      "terms": [{"c":200, "ids":[]}]
    }
  })";
  IsingCompact ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  ising.init();
  IsingCompactState state(0, 1);
  auto cost = ising.calculate_cost(state) + ising.get_const_cost();
  EXPECT_EQ(200, cost);
  auto result = ising.render_state(state);
  EXPECT_EQ(std::string("{}"), result.to_string());
}

class IsingCompactTest : public testing::Test
{
 public:
  IsingCompactTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input_str(R"({
      "cost_function": {
        "type": "ising",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [0, 1]},
          {"c": 1, "ids": [1, 2]},
          {"c": 1, "ids": [2, 3]},
          {"c": 1, "ids": [3, 4]},
          {"c": 1, "ids": [4, 5]},
          {"c": 1, "ids": [5, 6]},
          {"c": 1, "ids": [6, 7]},
          {"c": 1, "ids": [7, 8]},
          {"c": 1, "ids": [8, 9]},
          {"c": 1, "ids": [9, 0]}
        ]
      }
    })");
    utils::configure_with_configuration_from_json_string(input_str, ising);
    ising.init();
  }

  IsingCompact ising;
};

TEST_F(IsingCompactTest, SingleUpdates)
{
  IsingCompactState state(10, 10);

  double expected_cost = 10;
  EXPECT_EQ(ising.calculate_cost(state), expected_cost);
  for (int j = 0; j < 2; j++)
  {
    int transition = 0;  // flipping the first spin
    for (int i = 0; i < 10; i++)
    {
      double expected_diff = i == 0 ? -4 : 0;
      expected_diff = i == 9 ? 4 : expected_diff;
      EXPECT_EQ(ising.calculate_cost_difference(state, transition),
                expected_diff);

      ising.apply_transition(transition, state);  // Apply flip of first spin
      expected_cost = i == 9 ? 10 : 6;
      EXPECT_EQ(ising.calculate_cost(state), expected_cost);

      transition++;
    }
  }
}

TEST_F(IsingCompactTest, MinCostMinimum)
{
  double min_cost = ising.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ((MIN_DELTA_DEFAULT)*2, min_cost);
  double max_cost = ising.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(4, max_cost);
}

TEST(IsingCompact, SingleTerm)
{
  std::string input_str(R"({
      "cost_function": {
        "type": "ising",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [0]},
          {"c": 1, "ids": [1]},
          {"c": 1, "ids": [2]},
          {"c": 1, "ids": [3]},
          {"c": 1, "ids": [4]},
          {"c": 1, "ids": [5]},
          {"c": 1, "ids": [6]}
        ]
      }
    })");
  IsingCompact ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  ising.init();
  IsingCompactState state(7, 7);
  for (int i = 0; i < 7; i++)
  {
    state.spins[i] = true;
  }
  EXPECT_EQ(ising.calculate_cost(state), -7);
  uint64_t transition = 0;
  for (int i = 0; i < 7; i++)
  {
    EXPECT_EQ(ising.calculate_cost_difference(state, transition), 2);

    ising.apply_transition(transition, state);  // Apply flip of first spin
    EXPECT_EQ(ising.calculate_cost(state), (i + 1) * 2 - 7);

    transition++;
  }
  transition = 0;
  for (int i = 0; i < 7; i++)
  {
    EXPECT_EQ(ising.calculate_cost_difference(state, transition), -2);

    ising.apply_transition(transition, state);  // Apply flip of first spin
    EXPECT_EQ(ising.calculate_cost(state), 7 - 2 * (i + 1));

    transition++;
  }
}
template <typename Model_T>
void run_all()
{
  using ModelSolver = ::solver::ModelSolver<Model_T>;
  std::string input_str(R"({
      "cost_function": {
        "type": "ising",
        "version": "1.0",
        "terms": [
          {"c": -1, "ids": [0,1]},
          {"c": 1, "ids": [2]}
        ]
      }
    })");
  Model_T ising;
  utils::configure_with_configuration_from_json_string(input_str, ising);
  ising.init();
  auto params = utils::json_from_string(R"({"params": {"seed": 88}})");

  for (const auto& solver_name : SolversSupportMemSaving)
  {
    ModelSolver* solver =
        dynamic_cast<ModelSolver*>(create_solver<Model_T>(solver_name));
    EXPECT_NE(solver, nullptr);
    if (solver == nullptr) continue;
    solver->set_model(&ising);
    solver->configure(params);
    solver->init();
    solver->run();
    solver->finalize();
    utils::Structure result = solver->get_solutions();
    EXPECT_EQ(result["cost"].get<double>(), -2.0);
    EXPECT_EQ(result["configuration"]["0"].get<int>() *
                  result["configuration"]["1"].get<int>(),
              1);
    EXPECT_EQ(result["configuration"]["2"].get<int>(), -1);
  }
}

TEST(IsingCompact, AllSolvers8) { run_all<IsingCompact>(); }

TEST(IsingCompact, AllSolvers16) { run_all<IsingCompact16>(); }

TEST(IsingCompact, AllSolvers32) { run_all<IsingCompact32>(); }
