
#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/file.h"
#include "../../utils/proto_reader.h"
#include "../../utils/stream_handler_json.h"
#include "../../utils/stream_handler_proto.h"
#include "../ising.h"
#include "gtest/gtest.h"
#include "problem.pb.h"
#include "test_utils.h"
using ::utils::ConfigurationException;
using ::utils::Error;
using ::utils::Json;
using ::utils::Twister;
using ::utils::user_error;
using ::model::IsingTermCached;
using ::model::IsingTermCachedState;

class IsingTermCachedTest : public testing::Test
{
 public:
  IsingTermCachedTest()
  {
    // A ring of 10 spins with ferromagnetic 2-spin couplings.
    std::string input(R"({
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
    utils::configure_with_configuration_from_json_string(input, ising);
    ising.init();

    create_proto_test_problem(QuantumUtil::Problem_ProblemType_ISING);
    utils::configure_with_configuration_from_proto_folder(
        utils::data_path("models_input_problem_pb"), ising_proto);
    ising_proto.init();
  }

  IsingTermCached ising;
  IsingTermCached ising_proto;
};

TEST_F(IsingTermCachedTest, SingleUpdates)
{
  IsingTermCachedState state(10, 10);
  EXPECT_EQ(ising.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(ising.calculate_cost_difference(state, transition), -4);

  ising.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 6);

  EXPECT_EQ(ising.calculate_cost_difference(state, transition), 4);

  ising.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising.calculate_cost(state), 10);
}

TEST_F(IsingTermCachedTest, Metropolis)
{
  IsingTermCachedState state(10, 10);

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

TEST(IsingTermCached, VerifiesTypeLabelPresent)
{
  std::string input = R"({
    "cost_function": {
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  IsingTermCached ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input, ising),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(IsingTermCached, VerifiesTypeLabelValue)
{
  std::string input = R"({
    "cost_function": {
      "type": "foo",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  IsingTermCached ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input, ising),
      ConfigurationException,
      "parameter `type`: must be equal to 'ising', found 'foo'");
}

TEST(IsingTermCached, VerifiesVersionLabelPresent)
{
  std::string input = R"({
    "cost_function": {
      "type": "ising",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  IsingTermCached ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input, ising),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(IsingTermCached, VerifiesVersionLabelValue)
{
  std::string input = R"({
    "cost_function": {
      "type": "ising",
      "version": "-7",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  IsingTermCached ising;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(input, ising),
      ConfigurationException,
      "Expecting `version` equals 1.0 or 1.1, but found: -7");
}

TEST_F(IsingTermCachedTest, SingleUpdates_Proto)
{
  IsingTermCachedState state(10, 10);
  EXPECT_EQ(ising_proto.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(ising_proto.calculate_cost_difference(state, transition), -4);

  ising_proto.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising_proto.calculate_cost(state), 6);

  EXPECT_EQ(ising_proto.calculate_cost_difference(state, transition), 4);

  ising_proto.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(ising_proto.calculate_cost(state), 10);
}

TEST_F(IsingTermCachedTest, Metropolis_Proto)
{
  IsingTermCachedState state(10, 10);

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