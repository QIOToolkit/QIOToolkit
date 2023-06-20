// Copyright (c) Microsoft. All Rights Reserved.
#include "markov/state.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/config.h"
#include "utils/exception.h"
#include "utils/json.h"
#include "gtest/gtest.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::Structure;
using ::markov::State;

class TestState : public State
{
 public:
  TestState(std::string state) : state_(state) {}
  void configure(const utils::Json& json) override
  {
    this->param(json, "state", state_).description("test state").required();
  }
  Structure render() const override
  {
    Structure s;
    s = state_;
    return s;
  }
  utils::Structure get_status() const override { return state_; }

 private:
  std::string state_;
};

TEST(State, Serializes)
{
  TestState state("foo");
  EXPECT_EQ(state.render().to_string(), R"("foo")");
}

TEST(State, Debugs)
{
  TestState state("foo");
  std::stringstream s;
  s << state;
  EXPECT_EQ(s.str(), "<TestState: foo>");
}

TEST(State, Throws)
{
  TestState state("foo");
  std::string json = R"({})";
  auto in = utils::json_from_string(json);
  EXPECT_THROW_MESSAGE(state.configure(in), ConfigurationException,
                       "parameter `state` is required");
}
