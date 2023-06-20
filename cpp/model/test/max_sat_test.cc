// Copyright (c) Microsoft. All Rights Reserved.
#include "../max_sat.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/stream_handler_json.h"
#include "../../solver/all_solvers.h"
#include "gtest/gtest.h"

using ::utils::Dimacs;
using ::model::MaxSat32;
using ::model::MaxSat8;
using ::solver::create_solver;
using ModelSolver = ::solver::ModelSolver<MaxSat32>;

class MaxSatTest : public testing::Test
{
 public:
  MaxSatTest()
  {
    // 1(!x) + 4(x || y) 2(!y)
    std::string json(R"({
      "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [-1]},
          {"c": 4, "ids": [1, 2]},
          {"c": 2, "ids": [-2]}
        ]
      }
    })");
    utils::configure_with_configuration_from_json_string(json, maxsat1);
    maxsat1.init();

    Dimacs dimacs;
    dimacs.read(R"(c
c
c Weighted Max-SAT
c
p wcnf 2 3
1 -1 0
4 1 2 0
2 -2 0
)");
    maxsat2.configure(dimacs);
    maxsat2.init();
  }

  MaxSat32 maxsat1;
  MaxSat32 maxsat2;
};

TEST_F(MaxSatTest, CalculateCost)
{
  for (MaxSat32* maxsat : {&maxsat1, &maxsat2})
  {
    auto state = maxsat->create_state({0, 0});
    EXPECT_EQ(maxsat->calculate_cost(state), 4);
    state = maxsat->create_state({1, 0});
    EXPECT_EQ(maxsat->calculate_cost(state), 1);
    state = maxsat->create_state({0, 1});
    EXPECT_EQ(maxsat->calculate_cost(state), 2);
    state = maxsat->create_state({1, 1});
    EXPECT_EQ(maxsat->calculate_cost(state), 3);
  }
}

TEST_F(MaxSatTest, CalculateCostDifference)
{
  for (MaxSat32* maxsat : {&maxsat1, &maxsat2})
  {
    auto state = maxsat->create_state({0, 0});
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 0), 1 - 4);
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 1), 2 - 4);

    state = maxsat->create_state({0, 1});
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 0), 3 - 2);
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 1), 4 - 2);

    state = maxsat->create_state({1, 0});
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 0), 4 - 1);
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 1), 3 - 1);

    state = maxsat->create_state({1, 1});
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 0), 2 - 3);
    EXPECT_EQ(maxsat->calculate_cost_difference(state, 1), 1 - 3);
  }
}

TEST_F(MaxSatTest, ApplyTransition)
{
  for (MaxSat32* maxsat : {&maxsat1, &maxsat2})
  {
    auto state = maxsat->create_state({0, 0});
    EXPECT_EQ(state.variables, std::vector<bool>({0, 0}));
    maxsat->apply_transition(0, state);
    EXPECT_EQ(state.variables, std::vector<bool>({1, 0}));
    EXPECT_EQ(maxsat->calculate_cost(state), 1);

    maxsat->apply_transition(1, state);
    EXPECT_EQ(state.variables, std::vector<bool>({1, 1}));
    EXPECT_EQ(maxsat->calculate_cost(state), 3);

    maxsat->apply_transition(0, state);
    EXPECT_EQ(state.variables, std::vector<bool>({0, 1}));
    EXPECT_EQ(maxsat->calculate_cost(state), 2);

    maxsat->apply_transition(1, state);
    EXPECT_EQ(state.variables, std::vector<bool>({0, 0}));
    EXPECT_EQ(maxsat->calculate_cost(state), 4);
  }
}

TEST(MaxSat, ThrowsOnZeroVariable)
{
  std::string json(R"({
    "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [0]}
        ]
      }
    })");
  MaxSat32 maxsat;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, maxsat),
      std::exception,
      "MaxSat variables cannot have the name 0 since it cannot be negated to "
      "indicate 'not-0'.");
}

TEST(MaxSat, ThrowsOnIntMinVariable)
{
  std::string json(R"({
    "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 2, "ids": [-2147483648]}
        ]
      }
    })");
  MaxSat32 maxsat;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, maxsat),
      utils::ValueException,
      "MaxSat variables cannot have the name -2147483648 since it cannot be "
      "negated to indicate 'not -2147483648'.");
}

TEST(MaxSat, ThrowsOnEmptyClause)
{
  std::string json(R"({
    "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [1, 2]},
          {"c": 2, "ids": []}
        ]
      }
    })");
  MaxSat32 maxsat;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, maxsat),
      std::exception, "List of variables in a clause cannot be empty.");
}

TEST(MaxSat, HandlesRepeatedVariables)
{
  std::string json(R"({
    "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [1, 1]},
          {"c": 2, "ids": [-1, -1]}
        ]
      }
    })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  maxsat.init();
  auto one_true = maxsat.create_state({true});
  EXPECT_EQ(maxsat.calculate_cost(one_true), 2);
  EXPECT_EQ(maxsat.calculate_cost_difference(one_true, 0), -1);
  auto one_false = maxsat.create_state({false});
  EXPECT_EQ(maxsat.calculate_cost(one_false), 1);
  EXPECT_EQ(maxsat.calculate_cost_difference(one_false, 0), 1);
}

TEST(MaxSat, IgnoresAlwaysTrueClauses)
{
  std::string json(R"({
    "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [1]},
          {"c": 2, "ids": [1, -1]},
          {"c": 4, "ids": [-1, 1, -1]}
        ]
      }
    })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  maxsat.init();
  auto one_true = maxsat.create_state({true});
  EXPECT_EQ(maxsat.calculate_cost(one_true), 0);
  EXPECT_EQ(maxsat.calculate_cost_difference(one_true, 0), 1);
  auto one_false = maxsat.create_state({false});
  EXPECT_EQ(maxsat.calculate_cost(one_false), 1);
  EXPECT_EQ(maxsat.calculate_cost_difference(one_false, 0), -1);
}

TEST(MaxSat, HandlesFreeVariables)
{
  std::string json(R"({
    "cost_function": {
        "type": "maxsat",
        "version": "1.0",
        "terms": [
          {"c": 1, "ids": [1]},
          {"c": 2, "ids": [1, -1, 4, 2, 3]}
        ]
      }
    })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  maxsat.init();
  EXPECT_THROW_MESSAGE(maxsat.create_state({false, false, false, false}),
                       utils::ValueException,
                       "Wrong number of variables to initialize: got 4 but the "
                       "model has 1 active variable.");

  auto one_true = maxsat.create_state({true});
  EXPECT_EQ(maxsat.calculate_cost(one_true), 0);
  EXPECT_EQ(maxsat.calculate_cost_difference(one_true, 0), 1);

  auto one_false = maxsat.create_state({false});
  EXPECT_EQ(maxsat.calculate_cost(one_false), 1);
  EXPECT_EQ(maxsat.calculate_cost_difference(one_false, 0), -1);

  auto rendered = maxsat.render_state(one_false);
  EXPECT_EQ(rendered.get_type(), utils::Structure::OBJECT);
  EXPECT_EQ(rendered["1"].get<int>(), 0);
  EXPECT_EQ(rendered["2"].get<int>(), 1);
  EXPECT_EQ(rendered["3"].get<int>(), 1);
  EXPECT_EQ(rendered["4"].get<int>(), 1);
}

TEST(MaxSat, MaxCostPositive)
{
  std::string json(R"({
    "cost_function": {
      "type": "maxsat",
      "version": "1.0",
      "terms": [{"c":2, "ids":[1, 2]}, {"c":3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, -2]}]
    }
  })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  double max_cost = maxsat.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(9, max_cost);
}

TEST(MaxSat, MaxCostNegative)
{
  std::string json(R"({
    "cost_function": {
      "type": "maxsat",
      "version": "1.0",
      "terms": [{"c":2, "ids":[-1, 2]}, {"c":3, "ids":[-1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, -2]}]
    }
  })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  double max_cost = maxsat.estimate_max_cost_diff();
  EXPECT_DOUBLE_EQ(5, max_cost);
}

TEST(MaxSat, MinCost)
{
  std::string json(R"({
    "cost_function": {
      "type": "maxsat",
      "version": "1.0",
      "terms": [{"c":2, "ids":[1, 2]}, {"c":3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, -2]}]
    }
  })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  double min_cost = maxsat.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}

TEST(MaxSat, EstimatedMem)
{
  std::string json(R"({
    "cost_function": {
      "type": "maxsat",
      "version": "1.0",
      "terms": [{"c":2, "ids":[1, 2]}, {"c":3, "ids":[1]}, {"c":3, "ids":[1, 3]}, {"c":1, "ids":[1, -2]}]
    }
  })");
  MaxSat8 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  size_t states = maxsat.state_memory_estimate();
  size_t state_only = maxsat.state_only_memory_estimate();
  EXPECT_EQ(8, states);
  EXPECT_EQ(4, state_only);
}

TEST(MaxSat, MinCostEqual)
{
  std::string json(R"({
    "cost_function": {
      "type": "maxsat",
      "version": "1.0",
      "terms": [{"c":3, "ids":[-1, 2]}, {"c":3, "ids":[1,-2]}]
    }
  })");
  MaxSat32 maxsat;
  utils::configure_with_configuration_from_json_string(json, maxsat);
  double min_cost = maxsat.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(3, min_cost);
}

TEST_F(MaxSatTest, RunsWithSolvers)
{
  std::vector<std::string> solvers = {
      "microsoft.populationannealing-parameterfree.cpu",
      "microsoft.simulatedannealing-parameterfree.qiotoolkit",      
      "microsoft.substochasticmontecarlo-parameterfree.cpu",
      "microsoft.simulatedannealing.qiotoolkit",
      "microsoft.paralleltempering.qiotoolkit",
      "microsoft.populationannealing.cpu",
      "microsoft.substochasticmontecarlo.cpu"};
  auto params = utils::json_from_string(R"({"params": {"seed": 1}})");
  for (const auto& solver_name : solvers)
  {
    ModelSolver* solver =
        dynamic_cast<ModelSolver*>(create_solver<MaxSat32>(solver_name));
    EXPECT_NE(solver, nullptr);
    if (solver == nullptr) continue;
    solver->set_model(&maxsat1);
    solver->configure(params);
    solver->init();
    solver->run();
    solver->finalize();
    auto result = solver->get_solutions();
    EXPECT_EQ(result["cost"].get<double>(), 1.0);
    EXPECT_EQ(result["configuration"]["1"].get<int>(), 1);
    EXPECT_EQ(result["configuration"]["2"].get<int>(), 0);
  }
}
