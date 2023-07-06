
#include "model/clock.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "utils/stream_handler_json.h"
#include "gtest/gtest.h"

using ::utils::ConfigurationException;
using ::utils::Error;
using ::utils::Json;
using ::utils::Twister;
using ::utils::user_error;
using ::model::Clock;
using ::model::ClockState;
using ::model::ClockTransition;

class ClockTest : public testing::Test
{
 public:
  ClockTest()
  {
    // 2 spins with a single interaction term and 4 states each.
    utils::configure_with_configuration_from_json_string(R"(
      {
        "cost_function": {
          "type": "clock",
          "version": "1.0",
          "q": 4,
          "terms": [
            {"c": 1, "ids": [0, 1]}
          ]
        }
      }
    )",
                                                        clock2);
    // 4 spins with a single interaction term and 4 states each.
    utils::configure_with_configuration_from_json_string(R"(
      {
        "cost_function": {
          "type": "clock",
          "version": "1.0",
          "q": 4,
          "terms": [
            {"c": 1, "ids": [0, 1, 2, 3]}
          ]
        }
      }
    )",
                                                        clock4);
  }

  Clock clock2;
  Clock clock4;
};

TEST_F(ClockTest, Hamiltonian2)
{
  // This is the example from the comment in clock.cc
  ClockState state;

  // two aligned spins yield a coefficient of 1.0
  state.configure(utils::json_from_string(R"({"spins":[0,0]})"));
  clock2.initialize_terms(state);
  EXPECT_EQ(clock2.calculate_cost(state), 1.0);

  // two perpendicular spins yield a coefficient of 0.0
  state.configure(utils::json_from_string(R"({"spins": [0,1]})"));
  clock2.initialize_terms(state);
  EXPECT_EQ(clock2.calculate_cost(state), 0.0);

  // anti-aligned spins yield a coefficient of -1.0
  state.configure(utils::json_from_string(R"({"spins": [0,2]})"));
  clock2.initialize_terms(state);
  EXPECT_EQ(clock2.calculate_cost(state), -1.0);
}

TEST_F(ClockTest, Hamiltonian4)
{
  // This is the generalization to multi-spin interactions.
  ClockState state;

  // 4 spins aligned, sum=(4,0)
  state.configure(utils::json_from_string(R"({"spins":[0,0,0,0]})"));
  clock4.initialize_terms(state);
  EXPECT_EQ(clock4.calculate_cost(state), 1.0);

  // 2 and 2 perpendicular, sum=(2,2)
  state.configure(utils::json_from_string(R"({"spins": [0,0,1,1]})"));
  clock4.initialize_terms(state);
  EXPECT_EQ(clock4.calculate_cost(state), 0.0);

  // 2 and 2 anti-aligned, sum=(0,0)
  state.configure(utils::json_from_string(R"({"spins": [0,0,2,2]})"));
  clock4.initialize_terms(state);
  EXPECT_EQ(clock4.calculate_cost(state), -1.0);

  // all four directions, sum=(0,0)
  state.configure(utils::json_from_string(R"({"spins": [0,1,2,3]})"));
  clock4.initialize_terms(state);
  EXPECT_EQ(clock4.calculate_cost(state), -1.0);
}

TEST_F(ClockTest, SingleUpdates)
{
  ClockState state;
  auto input = utils::json_from_string(R"({"spins":[0,0,0,0]})");
  state.configure(input);
  clock4.initialize_terms(state);
  EXPECT_EQ(clock4.calculate_cost(state), 1.0);

  ClockTransition transition0 = {0, 2};  // flipping the first spin (0->2)
  EXPECT_EQ(clock4.calculate_cost_difference(state, transition0), -1.5);
  clock4.apply_transition(transition0, state);
  EXPECT_EQ(state.spins[0], 2);
  EXPECT_EQ(clock4.calculate_cost(state), -.5);

  ClockTransition transition1 = {1, 2};  // flip the second spin (0->2)
  EXPECT_EQ(clock4.calculate_cost_difference(state, transition1), -.5);
  clock4.apply_transition(transition1, state);
  EXPECT_EQ(state.spins[1], 2);
  EXPECT_EQ(clock4.calculate_cost(state), -1);
}

TEST_F(ClockTest, Metropolis)
{
  ClockState state;
  auto input = utils::json_from_string(R"({"spins":[0,0,0,0]})");
  state.configure(input);
  clock4.initialize_terms(state);
  EXPECT_EQ(clock4.calculate_cost(state), 1.0);

  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  double min = 1.0;
  double cost = 1.0;
  for (int i = 0; i < 100; i++)
  {
    auto transition = clock4.get_random_transition(state, rng);
    auto diff = clock4.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      clock4.apply_transition(transition, state);
      cost += diff;
    }
    min = std::min(min, cost);
  }
  EXPECT_EQ(min, -1);
}

TEST(Clock, VerifiesTypeLabelPresent)
{
  std::string json = R"(
    {
      "cost_function": {
        "version": "1.0",
        "q": 4, 
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(Clock, VerifiesTypeLabelValue)
{
  std::string json = R"(
    {
      "cost_function": {
        "type": "foo",
        "version": "1.0",
        "q": 4,
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException,
      "parameter `type`: must be equal to 'clock', found 'foo'");
}

TEST(Clock, VerifiesVersionLabelPresent)
{
  std::string json = R"(
    {
      "cost_function": {
        "type":"clock",
        "q": 4, 
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(Clock, VerifiesVersionLabelValue)
{
  std::string json = R"(
    {
      "cost_function": {
        "type": "clock",
        "version": "-7",
        "q": 4,
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException, "Expecting `version` equals 1.0, but found: -7");
}

TEST(Clock, VerifiesParameterQPresent)
{
  std::string json = R"(
    {
      "cost_function": {
        "type": "clock",
        "version": "1.0",
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException,
      "parameter `cost_function`: parameter `q` is required");
}

TEST(Clock, VerifiesParameterQIsInteger)
{
  std::string json = R"(
    {
      "cost_function": {
        "type": "clock",
        "version": "1.0",
        "q": 2.5,
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException,
      "parameter `cost_function`: "
      "parameter `q`: expected uint64, found 2.5 (double)");
}

TEST(Clock, VerifiesParameterQGreaterThan2)
{
  std::string json = R"(
    {
      "cost_function": { 
        "type": "clock",
        "version": "1.0",
        "q": 2,
        "terms":[{"c":1, "ids":[0,1]}]
      }
    }
  )";
  Clock clock;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, clock),
      ConfigurationException, "parameter `q`: must be greater than 2, found 2");
}
