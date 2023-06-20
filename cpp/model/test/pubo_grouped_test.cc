// Copyright (c) Microsoft. All Rights Reserved.
#include "model/pubo_grouped.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "utils/stream_handler_json.h"
#include "gtest/gtest.h"
#include "model/pubo.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::Twister;
using BinaryWithCounter = ::model::BinaryWithCounter<uint8_t>;
using PuboGrouped = ::model::PuboGrouped<uint8_t>;

class PuboGroupedTest : public testing::Test
{
 public:
  PuboGroupedTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input(R"({
      "cost_function": {
        "type": "pubo_grouped",
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
    utils::configure_with_configuration_from_json_string(input, pubo);
  }

  PuboGrouped pubo;
};

TEST_F(PuboGroupedTest, SingleUpdates)
{
  BinaryWithCounter state(10, 10);
  auto cost = pubo.calculate_cost(state);
  EXPECT_EQ(cost, 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(pubo.calculate_cost_difference(state, transition, &cost), -2);

  pubo.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 8);

  EXPECT_EQ(pubo.calculate_cost_difference(state, transition, &cost), 2);

  pubo.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 10);
}

TEST_F(PuboGroupedTest, Metropolis)
{
  BinaryWithCounter state(10, 10);

  auto cost = pubo.calculate_cost(state);
  EXPECT_EQ(cost, 10);  // Maximum value

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 10;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = pubo.get_random_transition(state, rng);
    auto diff = pubo.calculate_cost_difference(state, transition, &cost);
    if (double(diff) < 0 || rng.uniform() < exp(-diff / T))
    {
      pubo.apply_transition(transition, state, &cost);
      cost += diff;
    }
    min = std::min(min, double(cost));
  }
  EXPECT_EQ(min, 0);
}

TEST(PuboGrouped, VerifiesTypeLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "version":"1.0",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  PuboGrouped pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(PuboGrouped, VerifiesTypeLabelValue)
{
  std::string json = R"({
    "cost_function": {
      "type": "foo",
      "version":"1.0",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  PuboGrouped pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "parameter `type`: must be equal to 'pubo_grouped', found 'foo'");
}

TEST(PuboGrouped, VerifiesVersionLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "type":"pubo_grouped",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  PuboGrouped pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(PuboGrouped, VerifiesVersionLabelValue)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo_grouped",
      "version":"-7",
      "terms":[{"c":1, "ids":[0,1]}]
    }
  })";
  PuboGrouped pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "Expecting `version` equals 1.0 or 1.1, but found: -7");
}

TEST(PuboGrouped, DisorderedIDsInTerm)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo_grouped",
      "version":"1.1",
      "terms":[{"c":1, "ids":[0]}, {"c":2, "ids":[2,100]}]
    }
  })";
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(json, pubo);
  BinaryWithCounter state(3, 0);
  state.spins[0] = false;
  state.spins[1] = false;
  state.spins[2] = true;
  auto result = pubo.render_state(state);
  EXPECT_EQ(1, result["0"].get<int32_t>());
  EXPECT_EQ(1, result["2"].get<int32_t>());
  EXPECT_EQ(0, result["100"].get<int32_t>());
}

TEST(PuboGrouped, ConstantTerms)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo_grouped",
      "version":"1.1",
      "terms":[{"c":1, "ids":[]}, {"c":2, "ids":[]}]
    }
  })";
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(json, pubo);
  BinaryWithCounter state(0, 2);
  auto cost = pubo.calculate_cost(state);
  EXPECT_EQ(cost, 0);
  auto result = pubo.render_state(state);

  EXPECT_EQ(std::string("{}"), result.to_string());
}

TEST(PuboGrouped, ConstantTermsSLC)
{
  std::string input_oneconstant(R"({
      "cost_function": {
        "type": "pubo_grouped",
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
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(input_oneconstant, pubo);
  BinaryWithCounter state(2, 3);
  auto cost = pubo.calculate_cost(state);
  EXPECT_EQ(cost, 10);
  EXPECT_EQ(pubo.get_const_cost(), 18);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(pubo.calculate_cost_difference(state, transition, &cost), -10);

  pubo.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 0);

  EXPECT_EQ(pubo.calculate_cost_difference(state, transition, &cost), 10);

  pubo.apply_transition(transition, state, &cost);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 10);

  std::string input_multconstants(R"({
    "cost_function": {
      "type": "pubo_grouped",
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
  PuboGrouped pubo2;
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(
                           input_multconstants, pubo2),
                       ConfigurationException,
                       "Constant terms not combined in face with type slc!");
}

TEST(PuboGrouped, ConstantOnlyTermsSLC)
{
  std::string input_oneconstant(R"({
  "cost_function": {
    "version": "1.0",
    "type": "pubo_grouped",
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
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(input_oneconstant, pubo);
  BinaryWithCounter state(2, 3);
  auto cost = pubo.calculate_cost(state);
  EXPECT_EQ(cost + pubo.get_const_cost(),
            62);  // calculate_cost does not include the CONST values from non
                  // slc terms
}

TEST(PuboGrouped, NegativeIDs)
{
  std::string input(R"({
      "cost_function": {
        "type": "pubo_grouped",
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
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(input, pubo);
  BinaryWithCounter state(2, 3);
  auto cost = pubo.calculate_cost(state);
  EXPECT_EQ(cost, 20);

  // Flip the variable x_{-1}, which is the second internal spin
  // since ungrouped edges are processed as the first face
  size_t transition = 1;
  EXPECT_EQ(pubo.calculate_cost_difference(state, transition, &cost), -19);

  pubo.apply_transition(transition, state, &cost);
  EXPECT_EQ(pubo.calculate_cost(state), 1);

  EXPECT_EQ(pubo.calculate_cost_difference(state, transition, &cost), 19);

  pubo.apply_transition(transition, state, &cost);
  EXPECT_EQ(pubo.calculate_cost(state), 20);
}

TEST(PuboGrouped, NonlinearSLCException)
{
  std::string json_nonlinear(R"({
    "cost_function": {
      "type": "pubo_grouped",
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
  PuboGrouped nonlinear;
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(
                           json_nonlinear, nonlinear),
                       ConfigurationException,
                       "Face with slc type contains nonlinear edge.");
}

TEST(PuboGrouped, RegularCompareSLC)
{
  // Factored form: x0*x1 + x1*x2 + 2*(x1 + x2 - 1)^2
  std::string func_slc(R"({
    "cost_function": {
      "type": "pubo_grouped",
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
  std::string func_reg(R"({
    "cost_function": {
      "type": "pubo_grouped",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]},
        {"c": 2, "ids": [1, 1]},
        {"c": 4, "ids": [1, 2]},
        {"c": -4, "ids": [1]},
        {"c": 2, "ids": [2, 2]},
        {"c": -4, "ids": [2]},
        {"c": 2, "ids": []}
      ]
    }
  })");
  PuboGrouped pubo_grouped;
  utils::configure_with_configuration_from_json_string(func_slc, pubo_grouped);
  PuboGrouped pubo_reg;
  utils::configure_with_configuration_from_json_string(func_reg, pubo_reg);

  // Different representations yield different constant cost (in terms of
  // regular edges) Initial state
  BinaryWithCounter state_slc(3, 5);
  BinaryWithCounter state_reg(3, 7);
  auto cost_slc = pubo_grouped.calculate_cost(state_slc);
  auto cost_reg = pubo_reg.calculate_cost(state_reg);
  EXPECT_EQ(cost_slc, 4);
  EXPECT_EQ(cost_reg, 2);
  EXPECT_EQ(cost_slc.cache.size(), 2);

  // Spin 0
  auto diff = pubo_grouped.calculate_cost_difference(state_slc, 0, &cost_slc);
  EXPECT_EQ(diff, -1);
  EXPECT_EQ(pubo_reg.calculate_cost_difference(state_reg, 0, &cost_reg), -1);
  // Apply spin flip
  cost_slc += diff;
  pubo_grouped.apply_transition(0, state_slc, &cost_slc);
  pubo_reg.apply_transition(0, state_reg, &cost_reg);
  EXPECT_EQ(cost_slc, 3);
  EXPECT_EQ(pubo_grouped.calculate_cost(state_slc), 3);
  EXPECT_EQ(pubo_reg.calculate_cost(state_reg), 1);

  diff = pubo_grouped.calculate_cost_difference(state_slc, 0, &cost_slc);
  EXPECT_EQ(diff, 1);
  EXPECT_EQ(pubo_reg.calculate_cost_difference(state_reg, 0, &cost_reg), 1);
  // Apply spin flip again
  cost_slc += diff;
  pubo_grouped.apply_transition(0, state_slc, &cost_slc);
  pubo_reg.apply_transition(0, state_reg, &cost_reg);
  EXPECT_EQ(pubo_grouped.calculate_cost(state_slc), 4);
  EXPECT_EQ(cost_slc, 4);
  EXPECT_EQ(pubo_reg.calculate_cost(state_reg), 2);

  // Spin 1
  EXPECT_EQ(cost_slc.cache[1], 2);
  diff = pubo_grouped.calculate_cost_difference(state_slc, 1, &cost_slc);
  EXPECT_EQ(diff, -4);
  EXPECT_EQ(pubo_reg.calculate_cost_difference(state_reg, 1, &cost_reg), -4);
  // Apply spin flip
  cost_slc += diff;
  pubo_grouped.apply_transition(1, state_slc, &cost_slc);
  pubo_reg.apply_transition(1, state_reg, &cost_reg);
  EXPECT_EQ(pubo_grouped.calculate_cost(state_slc), 0);
  EXPECT_EQ(cost_slc, 0);
  EXPECT_EQ(pubo_reg.calculate_cost(state_reg), -2);

  diff = pubo_grouped.calculate_cost_difference(state_slc, 1, &cost_slc);
  EXPECT_EQ(diff, 4);
  EXPECT_EQ(pubo_reg.calculate_cost_difference(state_reg, 1, &cost_reg), 4);
  // Apply spin flip again
  cost_slc += diff;
  pubo_grouped.apply_transition(1, state_slc, &cost_slc);
  pubo_reg.apply_transition(1, state_reg, &cost_reg);
  EXPECT_EQ(pubo_grouped.calculate_cost(state_slc), 4);
  EXPECT_EQ(diff, 4);
  EXPECT_EQ(pubo_reg.calculate_cost(state_reg), 2);

  // Spin 2
  EXPECT_EQ(pubo_grouped.calculate_cost_difference(state_slc, 2, &cost_slc),
            -3);
  EXPECT_EQ(pubo_reg.calculate_cost_difference(state_reg, 2, &cost_reg), -3);
  // Apply spin flip
  pubo_grouped.apply_transition(2, state_slc, &cost_slc);
  pubo_reg.apply_transition(2, state_reg, &cost_reg);
  EXPECT_EQ(pubo_grouped.calculate_cost(state_slc), 1);
  EXPECT_EQ(pubo_reg.calculate_cost(state_reg), -1);

  EXPECT_EQ(pubo_grouped.calculate_cost_difference(state_slc, 2, &cost_slc), 3);
  EXPECT_EQ(pubo_reg.calculate_cost_difference(state_reg, 2, &cost_reg), 3);
  // Apply spin flip again
  pubo_grouped.apply_transition(2, state_slc, &cost_slc);
  pubo_reg.apply_transition(2, state_reg, &cost_reg);
  EXPECT_EQ(pubo_grouped.calculate_cost(state_slc), 4);
  EXPECT_EQ(pubo_reg.calculate_cost(state_reg), 2);
}

TEST(PuboGrouped, MetropolisSLC)
{
  // Factored form: x0*x1 + x1*x2 + x3 + 2*(x1 + x2 + x3 + x4 + x5 + x6 + x7 -
  // x9 - 1)^2 + 3*(x0 + 2*x2 + 4*x4 - x6 + x8)^2 initial cost is 200 minimum
  // cost is 0
  std::string func_slc(R"({
    "cost_function": {
      "type": "pubo_grouped",
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

  PuboGrouped pubo_grouped;
  utils::configure_with_configuration_from_json_string(func_slc, pubo_grouped);

  // Initial state
  BinaryWithCounter state_slc(10, 17);
  auto cost = pubo_grouped.calculate_cost(state_slc);

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(188);
  double min = 200;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = pubo_grouped.get_random_transition(state_slc, rng);
    auto diff =
        pubo_grouped.calculate_cost_difference(state_slc, transition, &cost);
    if (double(diff) < 0 || rng.uniform() < exp(-diff / T))
    {
      pubo_grouped.apply_transition(transition, state_slc, &cost);
      cost += diff;
    }
    min = std::min(min, double(cost));
  }
  EXPECT_EQ(min, 0);
}

TEST(PuboGrouped, Over255)
{
  std::string json_pre = R"({
    "cost_function": {
      "type": "pubo_grouped",
      "version":"1.1",
      "terms":[{"c":1, "ids":[)";
  std::string json_suf = R"(]}]}})";

  std::stringstream ss;
  for (int i = 0; i < 256; i++)
  {
    ss << i << ",";
  }
  ss << 256;

  std::string json = json_pre + ss.str() + json_suf;
  auto in = utils::json_from_string(json);
  ::model::PuboGrouped<uint32_t> pubo;
  pubo.configure(in);
  // no exception shall be thrown
}

TEST(PuboGrouped, StateVectorMemoryEstimate)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo_grouped",
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
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(input, pubo);

  size_t state_mem = pubo.state_memory_estimate();
  size_t expected_state_mem = sizeof(PuboGrouped::State_T) + 26 +
                              utils::vector_values_memory_estimate<bool>(2);
  EXPECT_EQ(state_mem, expected_state_mem);
}

TEST(PuboGrouped, MaxCostDiff)
{
  auto json = utils::json_from_string(R"({
    "cost_function": {
      "type": "pubo_grouped",
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
  PuboGrouped pubo;
  pubo.configure(json);
  double max_cost = pubo.estimate_max_cost_diff();
  EXPECT_EQ(8, max_cost);
}

TEST(PuboGrouped, MinCostDiff)
{
  std::string json(R"({
    "cost_function": {
      "type": "pubo_grouped",
      "version": "1.1",
      "terms": [
        {"c": 1, "ids": []},
        {"c": -4, "ids": [1]},
        {"c": -1, "ids": [2]},
        {"c": -1, "ids": [3]},
        {"c": 3, "ids": [1,2]},
        {"c": 1, "ids": [1,3]},
        {"c": 2, "ids": [2,3]}
      ],
      "terms_slc": [
        {"c": 1, "terms": [
          {"c": -1, "ids": [1]},
          {"c": 1, "ids": [2]},
          {"c": -1, "ids": [3]}
        ]}
  ]}})");
  PuboGrouped pubo;
  utils::configure_with_configuration_from_json_string(json, pubo);
  double min_cost = pubo.estimate_min_cost_diff();
  EXPECT_DOUBLE_EQ(1, min_cost);
}