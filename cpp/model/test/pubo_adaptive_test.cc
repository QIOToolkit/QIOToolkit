
#include "../pubo_adaptive.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/file.h"
#include "../../utils/stream_handler_json.h"
#include "gtest/gtest.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::Twister;
using Pubo = ::model::PuboAdaptive<uint16_t>;
using State = ::model::PuboBinaryAdaptive;

class PuboAdaptiveTest : public testing::Test
{
 public:
  PuboAdaptiveTest() {}

  Pubo pubo;
};

TEST_F(PuboAdaptiveTest, ConstantTerm)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": []},
        {"c": 2, "ids": []},
        {"c": 3, "ids": []}
      ]
    }
  })");
  utils::configure_with_configuration_from_json_string(input, pubo);
  pubo.init();
  model::Terms terms;
  utils::configure_with_configuration_from_json_string(input, terms);
  pubo.configure(std::move(terms), 8);
  pubo.init();

  State state(0);
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 6);
}

TEST_F(PuboAdaptiveTest, AssignmentConstruct)
{
  State one(1);
  State second(one);
}

TEST_F(PuboAdaptiveTest, LocalField)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [1]},
        {"c": 2, "ids": [2]},
        {"c": 3, "ids": [3]},
        {"c": 4, "ids": []}
      ]
    }
  })");
  utils::configure_with_configuration_from_json_string(input, pubo);
  model::Terms terms;
  utils::configure_with_configuration_from_json_string(input, terms);
  pubo.configure(std::move(terms), 8);

  State state(1);
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 10);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), -1);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -2);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -3);

  pubo.apply_transition(0, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 9);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), 1);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -2);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -3);

  pubo.apply_transition(0, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 10);
}

TEST_F(PuboAdaptiveTest, BinaryTerms)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0,1]},
        {"c": 2, "ids": [0,2]},
        {"c": 3, "ids": [1,2]},
        {"c": 4, "ids": [0]},
        {"c": 5, "ids": [1]},
        {"c": 6, "ids": [2]},
        {"c": 7, "ids": []}
      ]
    }
  })");
  utils::configure_with_configuration_from_json_string(input, pubo);
  model::Terms terms;
  utils::configure_with_configuration_from_json_string(input, terms);
  pubo.configure(std::move(terms), 8);

  State state(1);
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 28);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), -7);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -9);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -11);

  pubo.apply_transition(0, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 21);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), 7);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -8);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -9);

  pubo.apply_transition(2, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 12);
}

TEST_F(PuboAdaptiveTest, WithoutCaching)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0,1,2,3]},
        {"c": 2, "ids": [0,1,2]},
        {"c": 3, "ids": [0,1]},
        {"c": 4, "ids": [0,2]},
        {"c": 5, "ids": [1,2]},
        {"c": 6, "ids": [0]},
        {"c": 7, "ids": [1]},
        {"c": 8, "ids": [2]},
        {"c": 9, "ids": []}
      ]
    }
  })");
  utils::configure_with_configuration_from_json_string(input, pubo);
  model::Terms terms;
  utils::configure_with_configuration_from_json_string(input, terms);
  pubo.configure(std::move(terms), 8);  // limit state size to 8 bytes

  State state(1);
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 45);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), -16);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -18);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -20);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 3), -1);

  pubo.apply_transition(0, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 29);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), 16);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -12);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -13);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 3), 0);

  pubo.apply_transition(2, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 16);
}

TEST_F(PuboAdaptiveTest, WithSomeCaching)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0,1,2,3]},
        {"c": 2, "ids": [0,1,2]},
        {"c": 3, "ids": [0,1]},
        {"c": 4, "ids": [0,2]},
        {"c": 5, "ids": [1,2]},
        {"c": 6, "ids": [0]},
        {"c": 7, "ids": [1]},
        {"c": 8, "ids": [2]},
        {"c": 9, "ids": []}
      ]
    }
  })");
  utils::configure_with_configuration_from_json_string(input, pubo);
  model::Terms terms;
  utils::configure_with_configuration_from_json_string(input, terms);
  pubo.configure(std::move(terms), 12);  // limit state size to 12 bytes

  State state(2);
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 45);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), -16);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -18);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -20);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 3), -1);

  pubo.apply_transition(0, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 29);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), 16);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -12);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -13);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 3), 0);

  pubo.apply_transition(2, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 16);
}

TEST_F(PuboAdaptiveTest, WithFullCaching)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0,1,2,3]},
        {"c": 2, "ids": [0,1,2]},
        {"c": 3, "ids": [0,1]},
        {"c": 4, "ids": [0,2]},
        {"c": 5, "ids": [1,2]},
        {"c": 6, "ids": [0]},
        {"c": 7, "ids": [1]},
        {"c": 8, "ids": [2]},
        {"c": 9, "ids": []}
      ]
    }
  })");

  utils::configure_with_configuration_from_json_string(input, pubo);

  State state(3);
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 45);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), -16);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -18);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -20);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 3), -1);

  pubo.apply_transition(0, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 29);

  EXPECT_EQ(pubo.calculate_cost_difference(state, 0), 16);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 1), -12);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 2), -13);
  EXPECT_EQ(pubo.calculate_cost_difference(state, 3), 0);

  pubo.apply_transition(2, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state) + pubo.get_const_cost(), 16);
}

TEST_F(PuboAdaptiveTest, Metropolis)
{
  std::string input_file(utils::data_path("pubo_medium.json"));
  utils::configure_with_configuration_from_json_file(input_file, pubo);

  Twister rng;
  rng.seed(188);
  State state = pubo.get_random_state(rng);

  double cost = pubo.calculate_cost(state) + pubo.get_const_cost();
  EXPECT_EQ(cost, -17);

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  double min = cost;
  for (int i = 0; i < 100; i++)
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
  EXPECT_EQ(min, -17);
}

TEST_F(PuboAdaptiveTest, StateMemoryEstimate)
{
  std::string input(R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.0",
      "terms": [
        {"c": 1, "ids": [0,1,2,3]},
        {"c": 2, "ids": [0,1,2]},
        {"c": 3, "ids": [0,1]},
        {"c": 4, "ids": [0,2]},
        {"c": 5, "ids": [1,2]},
        {"c": 6, "ids": [0]},
        {"c": 7, "ids": [1]},
        {"c": 8, "ids": [2]},
        {"c": 9, "ids": []}
      ]
    }
  })");

  utils::configure_with_configuration_from_json_string(input, pubo);

  size_t state_mem = pubo.state_memory_estimate();
  size_t expected_state_mem = sizeof(State) + 2 * 8;
  EXPECT_EQ(state_mem, expected_state_mem);
}
