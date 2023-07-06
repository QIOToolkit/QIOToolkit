
#include "../pubo.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/operating_system.h"
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
using ::model::Pubo;
using PuboState = ::model::Binary;
using PuboCompact = ::model::PuboCompact<uint8_t>;
using PuboCompact16 = ::model::PuboCompact<uint16_t>;
using PuboCompact32 = ::model::PuboCompact<uint32_t>;
using ::model::PuboCompactState;
using ::solver::create_solver;

class PuboTest : public testing::Test
{
 public:
  PuboTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input_str(R"({
      "cost_function": {
        "type": "pubo",
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
          "0": 0, "1": 1, "2": 0, "3": 0, "4": 1,
          "5": 0, "6": 0, "7": 1, "8": 0, "9": 0
        }
      }
    })");
    utils::configure_with_configuration_from_json_string(input_str, pubo);

    create_proto_test_problem(QuantumUtil::Problem_ProblemType_PUBO);
    utils::configure_with_configuration_from_proto_folder(
        utils::data_path("models_input_problem_pb"), pubo_proto);
  }

  Pubo pubo;
  Pubo pubo_proto;
};

TEST_F(PuboTest, InitialConfiguration)
{
  PuboState state = pubo.get_initial_configuration_state();
  std::vector<bool> expected_spins = {
      true, false, true, true, false, 
      true, true,  false, true, true
  };
  EXPECT_EQ(state.spins, expected_spins);
}

TEST(Pubo, InitialConfigurationWrongValue)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}],
      "initial_configuration": {
          "0": 1, "1": -1
      }
    }
  })";
  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  EXPECT_THROW_MESSAGE(pubo.get_initial_configuration_state(),
                       utils::ValueException,
                       "parameter `initial_configuration` may have 1 and 0 "
                       "values for pubo models, found: -1");
}

TEST(Pubo, InitialConfigurationWrongNode)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}],
      "initial_configuration": {
          "0": 1, "2": 1
      }
    }
  })";
  Pubo pubo;
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(input_str, pubo),
                       utils::ValueException,
                       "parameter `initial_configuration` have node "
                       "name: 2 which is not found in model node names");
}

TEST_F(PuboTest, SingleUpdates)
{
  PuboState state(10, 10);
  EXPECT_EQ(pubo.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(pubo.calculate_cost_difference(state, transition), -2);

  pubo.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 8);

  EXPECT_EQ(pubo.calculate_cost_difference(state, transition), 2);

  pubo.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 10);
}

TEST_F(PuboTest, Metropolis)
{
  PuboState state(10, 10);

  EXPECT_EQ(pubo.calculate_cost(state), 10);  // Maximum value

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 10;
  double cost = 10;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = pubo.get_random_transition(state, rng);
    auto diff = pubo.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      pubo.apply_transition(transition, state);
      cost += diff;
    }
    min = std::min(min, cost);
  }
  EXPECT_EQ(min, 0);
}

TEST_F(PuboTest, EstimateCostDiff)
{
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_EQ(1, min_cost);
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_EQ(2, max_cost);
}

TEST_F(PuboTest, SingleUpdates_Proto)
{
  PuboState state(10, 10);
  EXPECT_EQ(pubo_proto.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(pubo_proto.calculate_cost_difference(state, transition), -2);

  pubo.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo_proto.calculate_cost(state), 8);

  EXPECT_EQ(pubo_proto.calculate_cost_difference(state, transition), 2);

  pubo.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo_proto.calculate_cost(state), 10);
}

TEST_F(PuboTest, Metropolis_Proto)
{
  PuboState state(10, 10);

  EXPECT_EQ(pubo_proto.calculate_cost(state), 10);  // Maximum value

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 10;
  double cost = 10;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = pubo_proto.get_random_transition(state, rng);
    auto diff = pubo_proto.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      pubo_proto.apply_transition(transition, state);
      cost += diff;
    }
    min = std::min(min, cost);
  }
  EXPECT_EQ(min, 0);
}

TEST_F(PuboTest, EstimateCostDiff_Proto)
{
  double min_cost = pubo_proto.estimate_min_cost_diff();
  EXPECT_EQ(1, min_cost);
  double max_cost = pubo_proto.estimate_max_cost_diff();
  EXPECT_EQ(2, max_cost);
}

TEST(Pubo, VerifiesTypeLabelPresent)
{
  std::string input_str = R"({
    "cost_function": {
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Pubo pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, pubo),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(Pubo, VerifiesTypeLabelValue)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "foo",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Pubo pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, pubo),
      ConfigurationException,
      "parameter `type`: must be equal to 'pubo', found 'foo'");
}

TEST(Pubo, VerifiesVersionLabelPresent)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Pubo pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, pubo),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(Pubo, VerifiesVersionLabelValue)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "-7",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  Pubo pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input_str, pubo),
      ConfigurationException,
      "Expecting `version` equals 1.0 or 1.1, but found: -7");
}

TEST(Pubo, DuplicatedIDsInTerm)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[-1,-1]}, {"c":2, "ids":[2,100]}]
    }
  })";
  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  PuboState state(3, 0);
  state.spins[0] = true;
  state.spins[1] = false;
  state.spins[2] = true;
  auto result = pubo.render_state(state);
  EXPECT_EQ(0, result["-1"].get<int32_t>());
  EXPECT_EQ(1, result["2"].get<int32_t>());
  EXPECT_EQ(0, result["100"].get<int32_t>());
}

TEST(Pubo, ConstantTerms)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-2, "ids":[]}, {"c":-2, "ids":[]}]
    }
  })";
  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  PuboState state(0, 3);

  auto cost = pubo.calculate_cost(state) + pubo.get_const_cost();
  EXPECT_EQ(-3, cost);
  auto result = pubo.render_state(state);

  QuantumUtil::Problem prob;
  EXPECT_EQ(std::string("{}"), result.to_string());
}

TEST(PuboState, BinaryMemoryEstimate)
{
  size_t n_nodes = 20000;
  size_t n_endges = 10000;
  size_t mem_estimate = PuboState::memory_estimate(n_nodes, n_endges);
  size_t expected_mem_estimate = sizeof(PuboState) + 2500;
  EXPECT_EQ(mem_estimate, expected_mem_estimate);
}

TEST(Pubo, StateVectorMemoryEstimate)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
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

  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);

  size_t state_mem = pubo.state_memory_estimate();
  size_t expected_state_mem = sizeof(PuboState) + 4;
  EXPECT_EQ(state_mem, expected_state_mem);
}

TEST(Pubo, MinCostMinimum)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-2, "ids":[1, 2]}, {"c":2, "ids":[1, 2]}]
    }
  })");
  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(MIN_DELTA_DEFAULT, min_cost);
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(4, max_cost);
}

TEST(Pubo, MinCost)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-2, "ids":[1, 2]}, {"c":-3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, 2]}]
    }
  })");
  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}

TEST(Pubo, MaxCost)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-2, "ids":[1, 2]}, {"c":-3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, 2]}]
    }
  })");
  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(9, max_cost);
}

TEST(Pubo, StateVectorMemoryEstimate_Proto)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
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

  Pubo pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);

  size_t state_mem = pubo.state_memory_estimate();
  size_t expected_state_mem = sizeof(PuboState) + 4;
  EXPECT_EQ(state_mem, expected_state_mem);
}

TEST(PuboCompact, MinCostMinimum)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-3, "ids":[1, 2]}, {"c":3, "ids":[1, 2]}]
    }
  })");
  PuboCompact pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(MIN_DELTA_DEFAULT, min_cost);
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(6, max_cost);
}

TEST(PuboCompact, MinCost)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-2, "ids":[1, 2]}, {"c":-3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, 2]}]
    }
  })");
  PuboCompact pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}

TEST(PuboCompact, MaxCost)
{
  std::string input_str(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":-2, "ids":[1, 2]}, {"c":-3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, 2]}]
    }
  })");
  PuboCompact pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(9, max_cost);
}

TEST(PuboCompact, DuplicatedIDsInTerm)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[-1,-1]}, {"c":9, "ids":[2,100]}, {"c":10, "ids":[3,3,3]}]
    }
  })";
  PuboCompact pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  PuboCompactState state(4, 0);
  state.spins[0] = true;
  state.spins[1] = false;
  state.spins[2] = true;
  state.spins[3] = true;
  auto result = pubo.render_state(state);
  EXPECT_EQ(0, result["-1"].get<int32_t>());
  EXPECT_EQ(1, result["2"].get<int32_t>());
  EXPECT_EQ(0, result["100"].get<int32_t>());
  EXPECT_EQ(0, result["3"].get<int32_t>());
}

TEST(PuboCompact, ConstantTerms)
{
  std::string input_str = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":9, "ids":[]}, {"c":-2, "ids":[]}, {"c":-2, "ids":[]}]
    }
  })";
  PuboCompact pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  PuboCompactState state(0, 3);

  auto cost = pubo.calculate_cost(state) + pubo.get_const_cost();
  EXPECT_EQ(5, cost);
  auto result = pubo.render_state(state);

  EXPECT_EQ(std::string("{}"), result.to_string());
}

class PuboCompactTest : public testing::Test
{
 public:
  PuboCompactTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input_str(R"({
      "cost_function": {
        "type": "pubo",
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
          {"c": 1, "ids": [9, 10]},
          {"c": 1, "ids": [10]}
        ]
      }
    })");
    utils::configure_with_configuration_from_json_string(input_str, pubo);
    pubo.init();
  }

  PuboCompact pubo;
};

TEST_F(PuboCompactTest, SingleUpdates)
{
  PuboCompactState state(11, 11);
  EXPECT_EQ(pubo.calculate_cost(state), 11);

  int transition = 0;  // flipping the first spin
  for (int i = 0; i < 11; i++)
  {
    EXPECT_EQ(pubo.calculate_cost_difference(state, transition), -1);

    pubo.apply_transition(transition, state);  // Apply flip of first spin
    EXPECT_EQ(pubo.calculate_cost(state), 10.0 - i);

    transition++;
  }
  transition = 0;

  for (int i = 0; i < 11; i++)
  {
    if (i == 0)
    {
      EXPECT_EQ(pubo.calculate_cost_difference(state, transition), 0);
    }
    else if (i == 10)
    {
      EXPECT_EQ(pubo.calculate_cost_difference(state, transition), 2);
    }
    else
    {
      EXPECT_EQ(pubo.calculate_cost_difference(state, transition), 1);
    }

    pubo.apply_transition(transition, state);  // Apply flip of first spin
    EXPECT_EQ(pubo.calculate_cost(state), i == 10 ? 11.0 : (double)i);

    transition++;
  }
}

TEST_F(PuboCompactTest, EstimateCostDiff)
{
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_EQ(1, min_cost);
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_EQ(2, max_cost);
}

TEST(PuboCompact, SingleTerm)
{
  std::string input_str(R"({
      "cost_function": {
        "type": "pubo",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [0]},
          {"c": 1, "ids": [1]},
          {"c": 1, "ids": [2]},
          {"c": 1, "ids": [3]},
          {"c": 1, "ids": [4]},
          {"c": 1, "ids": [5]},
          {"c": 1, "ids": [6]},
          {"c": 1, "ids": [7]}
        ]
      }
    })");
  PuboCompact pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  PuboCompactState state(8, 8);
  for (int i = 0; i < 8; i++)
  {
    state.spins[i] = true;
  }
  EXPECT_EQ(pubo.calculate_cost(state), 0);
  uint64_t transition = 0;
  for (int i = 0; i < 8; i++)
  {
    EXPECT_EQ(pubo.calculate_cost_difference(state, transition), 1);

    pubo.apply_transition(transition, state);  // Apply flip of first spin
    EXPECT_EQ(pubo.calculate_cost(state), i + 1);

    transition++;
  }
  transition = 0;
  for (int i = 0; i < 8; i++)
  {
    EXPECT_EQ(pubo.calculate_cost_difference(state, transition), -1);

    pubo.apply_transition(transition, state);  // Apply flip of first spin
    EXPECT_EQ(pubo.calculate_cost(state), 7 - i);

    transition++;
  }
}

template <typename Model_T>
void run_all_solvers()
{
  using ModelSolver = ::solver::ModelSolver<Model_T>;
  std::string input_str(R"({
      "cost_function": {
        "type": "pubo",
        "version": "1.0",
        "terms": [
          {"c": -1, "ids": [0,1]},
          {"c": 1, "ids": [1,2]}
        ]
      }
    })");
  Model_T pubo;
  utils::configure_with_configuration_from_json_string(input_str, pubo);
  pubo.init();
  auto params = utils::json_from_string(R"({"params": {"seed": 88}})");

  for (const auto& solver_name : SolversSupportMemSaving)
  {
    ModelSolver* solver =
        dynamic_cast<ModelSolver*>(create_solver<Model_T>(solver_name));
    EXPECT_NE(solver, nullptr);
    if (solver == nullptr) continue;
    solver->set_model(&pubo);
    solver->configure(params);
    solver->init();
    solver->run();
    solver->finalize();
    utils::Structure result = solver->get_solutions();
    EXPECT_EQ(result["cost"].get<double>(), -1.0);
    EXPECT_EQ(result["configuration"]["0"].get<int>(), 1);
    EXPECT_EQ(result["configuration"]["1"].get<int>(), 1);
    EXPECT_EQ(result["configuration"]["2"].get<int>(), 0);
  }
}

TEST(PuboCompact, AllSolvers8) { run_all_solvers<PuboCompact>(); }

TEST(PuboCompact, AllSolvers16) { run_all_solvers<PuboCompact16>(); }

TEST(PuboCompact, AllSolvers32) { run_all_solvers<PuboCompact32>(); }
