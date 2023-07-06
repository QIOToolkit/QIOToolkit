
#include "model/poly.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "utils/stream_handler_json.h"
#include "gtest/gtest.h"

using ::model::Poly;

TEST(Poly, SampleInput)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [
        {
          "parameter": "x",
          "constant": 2,
          "terms": [
            {"constant": -2, "ids": [0, 1]},
            {"constant": -1, "ids": [1, 2]},
            {"constant": -1, "ids": [2, 0]}
          ],
          "exponent": 2
        },
        {
          "parameter": "y",
          "constant": 3,
          "terms": [
            {"constant": -1, "ids": [0, 1]},
            {"constant": -2, "ids": [1, 2]},
            {"constant": -1, "ids": [2, 0]}
          ],
          "exponent": 2
        }
      ]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(
      poly.print(),
      "2x(-2s_0s_1 - s_1s_2 - s_0s_2)^2 + 3y(-s_0s_1 - 2s_1s_2 - s_0s_2)^2");

  auto state = poly.create_state({1, 1, 1});
  EXPECT_EQ(poly.calculate_cost(state), 80);  // 2*(-4)^2 + 3*(-4)^2

  auto flip0 = poly.create_spin_flip(0);
  EXPECT_EQ(poly.calculate_cost_difference(state, flip0), -72);
  poly.apply_transition(flip0, state);
  auto expected = poly.create_state({-1, 1, 1});
  EXPECT_EQ(state.spins, expected.spins);
  EXPECT_EQ(state.terms, expected.terms);

  EXPECT_EQ(poly.calculate_cost(state), 8);  // 2*(2)^2 + 3*(0)^2
  EXPECT_EQ(poly.calculate_cost_difference(state, flip0), +72);

  auto parameters = poly.get_parameters();
  EXPECT_EQ(parameters[0], "x");
  auto var_change = poly.create_parameter_change({3, 1});  // x: 1->3
  EXPECT_EQ(poly.calculate_cost_difference(state, var_change), +16);
  poly.apply_transition(var_change, state);
  EXPECT_EQ(poly.calculate_cost(state), 24);  // 2*3*(2)^2 + 3*(0)^2
}

TEST(Poly, Constant)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [{"constant": 42}]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(poly.print(), "42");

  auto state = poly.create_state({});
  EXPECT_EQ(poly.calculate_cost(state), 42);
}

TEST(Poly, SumOfConstants)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [{"constant": 13}, {"constant": 29}]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(poly.print(), "13 + 29");

  auto state = poly.create_state({});
  EXPECT_EQ(poly.calculate_cost(state), 42);
}

TEST(Poly, SumOfConstantAndSpin)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [{"constant": 43}, {"ids": [0]}]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(poly.print(), "43 + s_0");

  auto state = poly.create_state({-1});
  EXPECT_EQ(poly.calculate_cost(state), 42);

  auto spin_flip = poly.create_spin_flip(0);
  EXPECT_EQ(poly.calculate_cost_difference(state, spin_flip), 2);
  poly.apply_transition(spin_flip, state);
  EXPECT_EQ(poly.calculate_cost(state), 44);
}

TEST(Poly, Parameter)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [{"parameter": "x"}]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(poly.print(), "x");

  auto state = poly.create_state({});
  EXPECT_EQ(poly.calculate_cost(state), 1);

  auto parameter_change = poly.create_parameter_change({42});
  EXPECT_EQ(poly.calculate_cost_difference(state, parameter_change), 41);
  poly.apply_transition(parameter_change, state);
  EXPECT_EQ(poly.calculate_cost(state), 42);
}

TEST(Poly, ParameterAndSpin)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [{"parameter": "x"}, {"ids": [0]}]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(poly.print(), "x + s_0");

  auto state = poly.create_state({1});
  EXPECT_EQ(poly.calculate_cost(state), 2);

  auto parameter_change = poly.create_parameter_change({2});
  EXPECT_EQ(poly.calculate_cost_difference(state, parameter_change), 1);
  poly.apply_transition(parameter_change, state);
  EXPECT_EQ(poly.calculate_cost(state), 3);

  auto spin_flip = poly.create_spin_flip(0);
  EXPECT_EQ(poly.calculate_cost_difference(state, spin_flip), -2);
  poly.apply_transition(spin_flip, state);
  EXPECT_EQ(poly.calculate_cost(state), 1);
}

TEST(Poly, WeightedIsing)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [
        {
          "parameter": "x",
          "terms": [
            {"constant": -1, "ids": [0,1,2]},
            {"constant": -1, "ids": [2,3]},
            {"constant": -1, "ids": [3,0]}
          ]
        },
        {
          "parameter": "y",
          "terms": [
            {"constant": 2, "ids": [0]},
            {"constant": 3, "ids": [1,3]}
          ]
        }
      ]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  EXPECT_EQ(poly.print(),
            "x(-s_0s_1s_2 - s_2s_3 - s_0s_3) + y(2s_0 + 3s_1s_3)");

  auto state = poly.create_state({1, 1, 1, 1});
  // (-1 -1 -1) + (2 + 3)
  EXPECT_EQ(poly.calculate_cost(state), 2);

  auto parameter_change = poly.create_parameter_change({0, 1});
  EXPECT_EQ(poly.calculate_cost_difference(state, parameter_change), 3);
  poly.apply_transition(parameter_change, state);
  // 2 + 3
  EXPECT_EQ(poly.calculate_cost(state), 5);

  parameter_change = poly.create_parameter_change({1, 0});
  EXPECT_EQ(poly.calculate_cost_difference(state, parameter_change), -8);
  poly.apply_transition(parameter_change, state);
  // -1 -1 -1
  EXPECT_EQ(poly.calculate_cost(state), -3);
}

TEST(Poly, PenaltyTerm)
{
  std::string input(R"({
    "cost_function": {
      "type": "poly",
      "version": "0.1",
      "terms": [
        {
          "terms": [
            {"constant": 2, "ids": [0]},
            {"constant": 4, "ids": [1]},
            {"constant": 3, "ids": [2]}
          ]
        },
        {
          "constant": 10,
          "terms": [
            {"ids": [0]},
            {"ids": [1]},
            {"ids": [2]},
            {"constant": 1}
          ],
          "exponent": 2
        }
      ]
    }
  })");

  Poly poly;
  utils::configure_with_configuration_from_json_string(input, poly);
  // ising version of a penalty term for 1-hot encoding on 0-1-2
  // (exactly one should be positive to make the second parenthesis 0).
  EXPECT_EQ(poly.print(), "(2s_0 + 4s_1 + 3s_2) + 10(s_0 + s_1 + s_2 + 1)^2");

  auto state = poly.create_state({1, -1, -1});
  EXPECT_EQ(poly.calculate_cost(state), -5);
  state = poly.create_state({-1, 1, -1});
  EXPECT_EQ(poly.calculate_cost(state), -1);
  state = poly.create_state({-1, -1, 1});
  EXPECT_EQ(poly.calculate_cost(state), -3);

  state = poly.create_state({-1, -1, -1});
  EXPECT_EQ(poly.calculate_cost(state), 31);
  state = poly.create_state({1, 1, -1});
  EXPECT_EQ(poly.calculate_cost(state), 43);
  state = poly.create_state({1, 1, 1});
  EXPECT_EQ(poly.calculate_cost(state), 169);
}
