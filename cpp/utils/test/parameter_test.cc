// Copyright (c) Microsoft. All Rights Reserved.
#include <string>
#include <vector>

#include "utils/component.h"
#include "utils/config.h"
#include "utils/json.h"
#include "gtest/gtest.h"
#include "matcher/matchers.h"

using ::utils::Json;
using ::matcher::EqualTo;
using ::matcher::GreaterThan;
using ::matcher::IsSorted;

class ParameterTest : public testing::Test
{
 protected:
  utils::Component component;
};

TEST_F(ParameterTest, BoolParameter)
{
  auto json = utils::json_from_string(R"(
    {
      "yes": true,
      "no": false
    }
  )");

  bool yes = false;
  component.param(json, "yes", yes)
      .description("the 'yes' input is required")
      .required()
      .matches(EqualTo(true));
  EXPECT_TRUE(yes);

  bool no = true;
  component.param(json, "no", no)
      .description("the 'no' input is required")
      .required()
      .matches(EqualTo(false));
  EXPECT_FALSE(no);

  bool maybe = false;
  component.param(json, "yes", maybe)
      .description("Read a true value into the optional 'maybe'")
      .default_value(false);
  EXPECT_TRUE(maybe);

  component.param(json, "no", maybe)
      .description("Read a false value into the optional 'maybe'")
      .default_value(true);
  EXPECT_FALSE(maybe);

  component.param(json, "i_don_t_know", maybe)
      .description(
          "Read an unset value into the optional 'maybe', defaulting to true.")
      .default_value(true);
  EXPECT_TRUE(maybe);

  component.param(json, "can_you_repeat_the_question", maybe)
      .description(
          "Read an unset value into the optional 'maybe', defaulting to false")
      .default_value(false);
  EXPECT_FALSE(maybe);
}

TEST_F(ParameterTest, IntParameter)
{
  auto json = utils::json_from_string(R"(
    {
      "fortytwo": 42,
      "thirteen": 13
    }
  )");

  int fortytwo = 0;
  component.param(json, "fortytwo", fortytwo)
      .description("the 'fortytwo' input is required")
      .required()
      .matches(EqualTo(42));
  EXPECT_EQ(fortytwo, 42);

  int thirteen = 0;
  component.param(json, "thirteen", thirteen)
      .description("the 'thirteen' input is required")
      .required()
      .matches(EqualTo(13));
  EXPECT_EQ(thirteen, 13);

  int optional = 0;
  component.param(json, "fortytwo", optional)
      .description("Read fortytwo value into 'optional'")
      .default_value(11);
  EXPECT_EQ(optional, 42);

  component.param(json, "eleven", optional)
      .description("Read a false value into the optional 'maybe'")
      .default_value(11);
  EXPECT_EQ(optional, 11);
}

TEST_F(ParameterTest, StringParameter)
{
  auto json = utils::json_from_string(R"(
    {
      "hello": "world"
    }
  )");

  std::string hello;
  component.param(json, "hello", hello)
      .description("the 'world' input is required")
      .required();
  EXPECT_EQ(hello, "world");

  std::string optional;
  component.param(json, "hello", optional)
      .description("Read fortytwo value into 'optional'")
      .default_value("there");
  EXPECT_EQ(optional, "world");

  component.param(json, "hey", optional)
      .description("Read a false value into the optional 'maybe'")
      .default_value("there");
  EXPECT_EQ(optional, "there");
}
