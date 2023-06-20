// Copyright (c) Microsoft. All Rights Reserved.
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/file.h"
#include "../../utils/proto_reader.h"
#include "../../utils/stream_handler_json.h"
#include "../../utils/stream_handler_proto.h"
#include "../pubo.h"
#include "gtest/gtest.h"
#include "problem.pb.h"
#include "test_utils.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::Twister;
using BinaryWithCounter = ::model::BinaryWithCounter<uint8_t>;
using PuboWithCounter = ::model::PuboWithCounter<uint8_t>;

class PuboWithCounterTest : public testing::Test
{
 public:
  PuboWithCounterTest()
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
        ]
      }
    })");
    utils::configure_with_configuration_from_json_string(input_str, pubo);

    create_proto_test_problem(AzureQuantum::Problem_ProblemType_PUBO);
    utils::configure_with_configuration_from_proto_folder(
        utils::data_path("models_input_problem_pb"), pubo_proto);
  }

  PuboWithCounter pubo;
  PuboWithCounter pubo_proto;
};

TEST_F(PuboWithCounterTest, SingleUpdates)
{
  BinaryWithCounter state(10, 10);
  EXPECT_EQ(pubo.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(pubo.calculate_cost_difference(state, transition), -2);

  pubo.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 8);

  EXPECT_EQ(pubo.calculate_cost_difference(state, transition), 2);

  pubo.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo.calculate_cost(state), 10);
}

TEST_F(PuboWithCounterTest, Metropolis)
{
  BinaryWithCounter state(10, 10);

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
TEST_F(PuboWithCounterTest, SingleUpdates_Proto)
{
  BinaryWithCounter state(10, 10);
  EXPECT_EQ(pubo_proto.calculate_cost(state), 10);

  int transition = 0;  // flipping the first spin
  EXPECT_EQ(pubo_proto.calculate_cost_difference(state, transition), -2);

  pubo_proto.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo_proto.calculate_cost(state), 8);

  EXPECT_EQ(pubo_proto.calculate_cost_difference(state, transition), 2);

  pubo_proto.apply_transition(transition, state);  // Apply flip of first spin
  EXPECT_EQ(pubo_proto.calculate_cost(state), 10);
}

TEST_F(PuboWithCounterTest, Metropolis_Proto)
{
  BinaryWithCounter state(10, 10);

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

TEST(PuboWithCounter, VerifiesTypeLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  PuboWithCounter pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "parameter `cost_function`: parameter `type` is required");
}

TEST(PuboWithCounter, VerifiesTypeLabelValue)
{
  std::string json = R"({
    "cost_function": {
      "type": "foo",
      "version": "1.0",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  PuboWithCounter pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "parameter `type`: must be equal to 'pubo', found 'foo'");
}

TEST(PuboWithCounter, VerifiesVersionLabelPresent)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  PuboWithCounter pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "parameter `cost_function`: parameter `version` is required");
}

TEST(PuboWithCounter, VerifiesVersionLabelValue)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "version": "-7",
      "terms": [{"c":1, "ids":[0,1]}]
    }
  })";
  PuboWithCounter pubo;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json, pubo),
      ConfigurationException,
      "Expecting `version` equals 1.0 or 1.1, but found: -7");
}

TEST(PuboWithCounter, DisorderedIDsInTerm)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[0]}, {"c":2, "ids":[2,100]}]
    }
  })";
  PuboWithCounter pubo;
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

TEST(PuboWithCounter, ConstantTerms)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[]}, {"c":2, "ids":[]}]
    }
  })";
  PuboWithCounter pubo;
  utils::configure_with_configuration_from_json_string(json, pubo);
  BinaryWithCounter state(0, 2);
  auto cost = pubo.calculate_cost(state) + pubo.get_const_cost();
  EXPECT_EQ(3, cost);
  auto result = pubo.render_state(state);

  EXPECT_EQ(std::string("{}"), result.to_string());
}

TEST(PuboWithCounter, Over255)
{
  std::string json_pre = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[)";

  std::string json_suf = R"(]}]}})";

  std::stringstream ss;
  for (int i = 0; i < 256; i++)
  {
    ss << i << ",";
  }
  ss << 256;

  std::string json = json_pre + ss.str() + json_suf;
  ::model::PuboWithCounter<uint32_t> pubo;
  utils::configure_with_configuration_from_json_string(json, pubo);
  // no exception shall be thrown
}
