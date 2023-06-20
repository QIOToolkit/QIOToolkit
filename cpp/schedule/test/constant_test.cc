// Copyright (c) Microsoft. All Rights Reserved.
#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "schedule/schedule.h"

using ::utils::Json;
using ::schedule::Schedule;
using testing::DoubleEq;
using testing::ElementsAre;

TEST(Schedule, Constant)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "constant",
    "value": 42
  })");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(-1), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(0), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.5), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 42);
}

TEST(Schedule, ConstantStartstop)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "constant",
    "value": 42,
    "start": 2,
    "stop": 4
  })");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(3), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(4), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(5), 42);
}

TEST(Schedule, ConstantRepeat)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "constant",
    "value": 42,
    "start": 2,
    "stop": 4,
    "repeat": true
  })");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(3), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(4), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(5), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(6), 42);
  EXPECT_DOUBLE_EQ(schedule.get_value(7), 42);
}
