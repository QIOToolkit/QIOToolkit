
#include "model/ising_grouped.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "utils/stream_handler_json.h"
#include "gtest/gtest.h"
#include "model/ising.h"

using ::utils::ConfigurationException;
using ::utils::Error;
using ::utils::Json;
using ::utils::Twister;
using ::utils::user_error;
using ::model::IsingGrouped;
using ::model::IsingTermCached;
using ::model::IsingTermCachedState;

class IsingGroupedTest : public testing::Test
{
 public:
  IsingGroupedTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input(R"({
      "cost_function": {
        "type": "ising_grouped",
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
    utils::configure_with_configuration_from_json_string(input, ising);
  }

  IsingGrouped ising;
};

TEST_F(IsingGroupedTest, SingleUpdates)
{
  IsingTermCachedState state(10, 10);
  auto cost = ising.calculate_cost(state);
  EXPECT_EQ(cost, 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(ising.calculate_cost_difference(state, transition, &cost), -4);

  ising.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 6);

  EXPECT_EQ(ising.calculate_cost_difference(state, transition, &cost), 4);

  ising.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 10);
}

TEST_F(IsingGroupedTest, Metropolis)
{
  IsingTermCachedState state(10, 10);
  auto cost = ising.calculate_cost(state);
  EXPECT_EQ(cost, 10);  // Maximum value

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 10;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = ising.get_random_transition(state, rng);
    auto diff = ising.calculate_cost_difference(state, transition, &cost);
    if (double(diff) < 0 || rng.uniform() < exp(-diff / T))
    {
      ising.apply_transition(transition, state, &cost);
      cost += diff;
    }
    min = std::min(min, double(cost));
  }
  EXPECT_EQ(min, -10);
}

TEST(IsingGrouped, VerifiesTypeLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "version":"1.0",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  IsingGrouped ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, ising),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(IsingGrouped, VerifiesTypeLabelValue)
{
  std::string json = R"({
    "cost_function": {
      "type": "foo",
      "version":"1.0",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  IsingGrouped ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, ising),
      ConfigurationException,
      "parameter `type`: must be equal to 'ising_grouped', found 'foo'");
}

TEST(IsingGrouped, VerifiesVersionLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "type":"ising_grouped",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  IsingGrouped ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, ising),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(IsingGrouped, VerifiesVersionLabelValue)
{
  std::string json = R"({
    "cost_function": {
      "type": "ising_grouped",
      "version":"-7",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  auto in = utils::json_from_string(json);
  IsingGrouped ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, ising),
      ConfigurationException,
      "Expecting `version` equals 1.0 or 1.1, but found: -7");
}

TEST(IsingGrouped, ConstantTerms)
{
  std::string json = R"({
    "cost_function": {
      "type": "ising_grouped",
      "version":"1.1",
      "terms":[{"c":1, "ids":[]}, {"c":2, "ids":[]}]
    }
  })";
  auto in = utils::json_from_string(json);
  IsingGrouped ising = IsingGrouped();
  ising.configure(in);
  IsingTermCachedState state(0, 2);
  auto cost = ising.calculate_cost(state);
  EXPECT_EQ(cost, 0);
  auto result = ising.render_state(state);

  EXPECT_EQ(std::string("{}"), result.to_string());
}

TEST(IsingGrouped, ConstantTermsSLC)
{
  std::string input_oneconstant(R"({
      "cost_function": {
        "type": "ising_grouped",
        "version": "1.0",
        "terms_slc": [
          {"c": 2, "terms": [
            {"c": -3, "ids": []}
          ]},
          {"c": 1, "terms": [
            {"c": -3, "ids": [0]}
          ]}
        ],
        "terms": [
          {"c": 1, "ids": [0, 1]}
        ]
      }
    })");
  IsingGrouped ising;
  utils::configure_with_configuration_from_json_string(input_oneconstant, ising);
  IsingTermCachedState state(2, 3);
  auto cost = ising.calculate_cost(state);
  EXPECT_EQ(cost, 10);
  EXPECT_EQ(ising.get_const_cost(), 18);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(ising.calculate_cost_difference(state, transition, &cost), -2);

  ising.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 8);

  EXPECT_EQ(ising.calculate_cost_difference(state, transition, &cost), 2);

  ising.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 10);

  std::string input_multconstants(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms_slc": [
        {"c": 1, "terms": [
          {"c": -3, "ids": []},
          {"c": 1, "ids": []}
        ]}
       ],
      "terms": [
        {"c": 1, "ids": [0, 1]}
      ]
    }
  })");
  IsingGrouped ising2;
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(
                           input_multconstants, ising2),
                       ConfigurationException,
                       "Constant terms not combined in face with type slc!");
}

TEST(IsingGrouped, ConstantOnlyTermsSLC)
{
  std::string input_oneconstant(R"({
  "cost_function": {
    "version": "1.0",
    "type": "ising_grouped",
    "terms_slc": [
      {
        "c": 1,
        "terms": [
          {
              "c": 8,
              "ids": [
              ]
          }
        ]
      }
    ],
    "terms": [

      {
        "c": -2,
        "ids": [
        ]
      }
    ]
  }
  })");
  IsingGrouped ising;
  utils::configure_with_configuration_from_json_string(input_oneconstant, ising);
  IsingTermCachedState state(2, 3);
  auto cost = ising.calculate_cost(state);
  EXPECT_EQ(cost + ising.get_const_cost(), 62);
}

TEST(IsingGrouped, NegativeIDs)
{
  std::string input(R"({
      "cost_function": {
        "type": "ising_grouped",
        "version": "1.0",
        "terms_slc": [
          {"c": 2, "terms": [
            {"c": -3, "ids": [-1]}
          ]}
         ],
        "terms": [
          {"c": 1, "ids": [0, -1]},
          {"c": 1, "ids": [0]}
        ]
      }
    })");
  IsingGrouped ising;
  utils::configure_with_configuration_from_json_string(input, ising);
  IsingTermCachedState state(2, 3);
  auto cost = ising.calculate_cost(state);
  EXPECT_EQ(cost, 20);

  // Flip the variable x_{-1}, which is the second internal spin
  // since ungrouped edges are processed as the first face
  size_t transition = 1;
  EXPECT_EQ(ising.calculate_cost_difference(state, transition, &cost), -2);

  ising.apply_transition(transition, state, &cost);
  EXPECT_EQ(ising.calculate_cost(state), 18);

  EXPECT_EQ(ising.calculate_cost_difference(state, transition, &cost), 2);

  ising.apply_transition(transition, state, &cost);
  EXPECT_EQ(ising.calculate_cost(state), 20);
}

TEST(IsingGrouped, NonlinearSLCException)
{
  std::string json_nonlinear(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms": [
        {"c": 1.1, "ids": [0,1]},
        {"c": 2.1, "ids": [0,2]}
      ],
      "terms_slc": [
        {"c": 2.0, "terms": [
          {"c": 2.9, "ids": [1]},
          {"c": 8.0, "ids": []}
        ]},
        {"c": 2.0, "terms": [
          {"c": 1.2, "ids": [1,2]},
          {"c": 1.3, "ids": [2]},
          {"c": 1.4, "ids": [3]}
        ]}
      ]
    }
  })");
  IsingGrouped nonlinear;
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(
                           json_nonlinear, nonlinear),
                       ConfigurationException,
                       "Face with slc type contains nonlinear edge.");
}

TEST(IsingGrouped, RegularCompareSLC)
{
  // Factored form: x0*x1 + x1*x2 + 2*(x1 + x2 - 1)^2
  std::string func_slc(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]}
      ],
      "terms_slc": [
        {"c": 2, "terms": [
          {"c": 1, "ids": [1]},
          {"c": 1, "ids": [2]},
          {"c": -1, "ids": []}]}
      ]
    }
  })");

  // Expanded form: x0*x1 + x1*x2 + 2*x1^2 + 4*x1*x2 - 4*x1 + 2*x2^2 - 4*x2 + 2
  // Ising-simplified to x0*x1 + x1*x2 + 2 + 4*x1*x2 - 4*x1 + 2 - 4*x2 + 2
  std::string func_reg(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]},
        {"c": 2, "ids": []},
        {"c": 4, "ids": [1, 2]},
        {"c": -4, "ids": [1]},
        {"c": 2, "ids": []},
        {"c": -4, "ids": [2]},
        {"c": 2, "ids": []}
      ]
    }
  })");
  IsingGrouped ising_grouped;
  utils::configure_with_configuration_from_json_string(func_slc, ising_grouped);
  IsingGrouped ising_reg;
  utils::configure_with_configuration_from_json_string(func_reg, ising_reg);

  // Different representations yield different constant cost (in terms of
  // regular edges) Initial state
  IsingTermCachedState state_slc(3, 5);
  IsingTermCachedState state_reg(3, 5);
  auto cost_slc = ising_grouped.calculate_cost(state_slc);
  EXPECT_EQ(ising_grouped.get_term(state_slc, 0), 1);
  EXPECT_EQ(ising_grouped.get_term(state_slc, 1), 1);
  EXPECT_EQ(ising_grouped.get_term(state_slc, 2), 1);
  EXPECT_EQ(ising_grouped.get_term(state_slc, 3), 1);
  EXPECT_EQ(ising_grouped.get_term(state_slc, 4), -1);
  auto cost_reg = ising_reg.calculate_cost(state_reg);
  EXPECT_EQ(cost_slc, 4);
  EXPECT_EQ(cost_reg, -2);
  EXPECT_EQ(cost_slc.cache.size(), 2);
  EXPECT_EQ(cost_slc.cache[1], 2);

  // Spin 0
  auto diff = ising_grouped.calculate_cost_difference(state_slc, 0, &cost_slc);
  EXPECT_EQ(diff, -2);
  EXPECT_EQ(ising_reg.calculate_cost_difference(state_reg, 0, &cost_reg), -2);
  // Apply spin flip
  cost_slc += diff;
  EXPECT_EQ(cost_slc, 2);
  ising_grouped.apply_transition(0, state_slc, &cost_slc);
  ising_reg.apply_transition(0, state_reg, &cost_reg);
  EXPECT_EQ(ising_grouped.calculate_cost(state_slc), 2);
  EXPECT_EQ(ising_reg.calculate_cost(state_reg), -4);

  diff = ising_grouped.calculate_cost_difference(state_slc, 0, &cost_slc);
  EXPECT_EQ(diff, 2);
  EXPECT_EQ(ising_reg.calculate_cost_difference(state_reg, 0, &cost_reg), 2);
  // Apply spin flip again
  cost_slc += diff;
  EXPECT_EQ(cost_slc, 4);
  ising_grouped.apply_transition(0, state_slc, &cost_slc);
  ising_reg.apply_transition(0, state_reg, &cost_reg);
  EXPECT_EQ(ising_grouped.calculate_cost(state_slc), 4);
  EXPECT_EQ(ising_reg.calculate_cost(state_reg), -2);

  // Spin 1
  EXPECT_EQ(cost_slc.cache[1], 2);
  diff = ising_grouped.calculate_cost_difference(state_slc, 1, &cost_slc);
  EXPECT_EQ(diff, -4);
  EXPECT_EQ(ising_reg.calculate_cost_difference(state_reg, 1, &cost_reg), -4);
  // Apply spin flip
  cost_slc += diff;
  EXPECT_EQ(cost_slc, 0);
  ising_grouped.apply_transition(1, state_slc, &cost_slc);
  ising_reg.apply_transition(1, state_reg, &cost_reg);
  EXPECT_EQ(ising_grouped.calculate_cost(state_slc), 0);
  EXPECT_EQ(ising_reg.calculate_cost(state_reg), -6);

  diff = ising_grouped.calculate_cost_difference(state_slc, 1, &cost_slc);
  EXPECT_EQ(diff, 4);
  EXPECT_EQ(ising_reg.calculate_cost_difference(state_reg, 1, &cost_reg), 4);
  // Apply spin flip again
  cost_slc += diff;
  EXPECT_EQ(cost_slc, 4);
  ising_grouped.apply_transition(1, state_slc, &cost_slc);
  ising_reg.apply_transition(1, state_reg, &cost_reg);
  EXPECT_EQ(ising_grouped.calculate_cost(state_slc), 4);
  EXPECT_EQ(ising_reg.calculate_cost(state_reg), -2);

  // Spin 2
  EXPECT_EQ(ising_grouped.calculate_cost_difference(state_slc, 2, &cost_slc),
            -2);
  EXPECT_EQ(ising_reg.calculate_cost_difference(state_reg, 2, &cost_reg), -2);
  // Apply spin flip
  ising_grouped.apply_transition(2, state_slc, &cost_slc);
  ising_reg.apply_transition(2, state_reg, &cost_reg);
  EXPECT_EQ(ising_grouped.calculate_cost(state_slc), 2);
  EXPECT_EQ(ising_reg.calculate_cost(state_reg), -4);

  EXPECT_EQ(ising_grouped.calculate_cost_difference(state_slc, 2, &cost_slc),
            2);
  EXPECT_EQ(ising_reg.calculate_cost_difference(state_reg, 2, &cost_reg), 2);
  // Apply spin flip again
  ising_grouped.apply_transition(2, state_slc, &cost_slc);
  ising_reg.apply_transition(2, state_reg, &cost_reg);
  EXPECT_EQ(ising_grouped.calculate_cost(state_slc), 4);
  EXPECT_EQ(ising_reg.calculate_cost(state_reg), -2);
}

TEST(IsingGrouped, MetropolisSLC)
{
  // Factored form: x0*x1 + x1*x2 + x3 + 2*(x1 + x2 + x3 + x4 + x5 + x6 + x7 -
  // x9 - 1)^2 + 3*(x0 + 2*x2 + 4*x4 - x6 + x8)^2 initial cost is 150 minimum
  // cost is 2
  std::string func_slc(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]},
        {"c": 1, "ids": [3]}
      ],
      "terms_slc": [
        {"c": 2, "terms": [
          {"c": 1, "ids": [1]},
          {"c": 1, "ids": [2]},
          {"c": 1, "ids": [3]},
          {"c": 1, "ids": [4]},
          {"c": 1, "ids": [5]},
          {"c": 1, "ids": [6]},
          {"c": 1, "ids": [7]},
          {"c": -1, "ids": [9]},
          {"c": -1, "ids": []}
        ]},
        {"c": 3, "terms": [
          {"c": 1, "ids": [0]},
          {"c": 2, "ids": [2]},
          {"c": 4, "ids": [4]},
          {"c": -1, "ids": [6]},
          {"c": 1, "ids": [8]}
        ]}
      ]
    }
  })");

  IsingGrouped ising_grouped;
  utils::configure_with_configuration_from_json_string(func_slc, ising_grouped);

  // Initial state
  IsingTermCachedState state_slc(10, 17);
  auto cost = ising_grouped.calculate_cost(state_slc);

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 200;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = ising_grouped.get_random_transition(state_slc, rng);
    auto diff =
        ising_grouped.calculate_cost_difference(state_slc, transition, &cost);
    if (double(diff) < 0 || rng.uniform() < exp(-diff / T))
    {
      ising_grouped.apply_transition(transition, state_slc, &cost);
      cost += diff;
    }
    min = std::min(min, double(cost));
  }
  EXPECT_EQ(min, 2);
}

TEST(IsingGrouped, StateVectorMemoryEstimate)
{
  std::string input(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]},
        {"c": 1, "ids": [2, 3]},
        {"c": 1, "ids": [3, 4]},
        {"c": 1, "ids": [4, 5]}
      ],
      "terms_slc": [
        {"c": 3, "terms": [
          {"c": 1, "ids": [0]},
          {"c": 2, "ids": [2]},
          {"c": 4, "ids": [4]},
          {"c": -1, "ids": [6]},
          {"c": 1, "ids": [8]}
        ]}
      ]
    }
  })");
  IsingGrouped ising;
  utils::configure_with_configuration_from_json_string(input, ising);

  size_t state_mem = ising.state_memory_estimate();
  size_t expected_state_mem = sizeof(IsingGrouped::State_T) + 20 +
                              utils::vector_values_memory_estimate<bool>(2);
  EXPECT_EQ(state_mem, expected_state_mem);
}

TEST(IsingGrouped, MaxCostDiff)
{
  std::string json(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.1",
      "terms": [
        {"ids":[0,1], "c":-1},
        {"ids":[1,2], "c":-3},
        {"ids":[0,2], "c":-4},
        {"ids":[0], "c":1},
        {"ids":[1], "c":1},
        {"ids":[2], "c":6},
        {"ids":[], "c":99999}
      ],
      "terms_slc": [
        {"c":1, "terms": [
          {"ids":[0], "c":1},
          {"ids":[1], "c":1},
          {"ids":[2], "c":1},
          {"ids":[], "c":-1}
        ]}
  ]}})");
  IsingGrouped ising;
  utils::configure_with_configuration_from_json_string(json, ising);
  double max_cost = ising.estimate_max_cost_diff();
  EXPECT_EQ(8, max_cost);
}

TEST(IsingGrouped, MinCostDiff)
{
  std::string input(R"({
    "cost_function": {
      "type": "ising_grouped",
      "version": "1.0",
      "terms": [
        {"c": -1, "ids": [0, 1]},
        {"c": -2, "ids": [1, 2]},
        {"c": 3, "ids": [2, 3]},
        {"c": -4, "ids": [3, 4]},
        {"c": 5, "ids": [4, 5]},
        {"c": -6, "ids": [5, 6]},
        {"c": 7, "ids": [6, 7]},
        {"c": -8, "ids": [7, 8]},
        {"c": 9, "ids": [8, 9]},
        {"c": -1.5, "ids": [9, 0]},
        {"c": 8, "ids": [1, 9]},
        {"c": -18, "ids": []}
      ],
      "terms_slc": [
        {"c": 1, "terms": [
          {"c": 1, "ids": [0]},
          {"c": 1, "ids": [1]},
          {"c": -4, "ids": [9]}
        ]}
      ]
    }
  })");
  IsingGrouped ising;
  utils::configure_with_configuration_from_json_string(input, ising);
  double min_cost = ising.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}