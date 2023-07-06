
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

TEST(Schedule, Linear)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "linear",
    "initial": 3,
    "final": 5
  })");
  schedule.configure(input);
  //    |   _____
  //    |  /
  //    | /
  // ___|/
  //    |
  // ---+---+-----
  //    0   1
  EXPECT_DOUBLE_EQ(schedule.get_value(-1), 3);
  EXPECT_DOUBLE_EQ(schedule.get_value(0), 3);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.5), 4);
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 5);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 5);
}

TEST(Schedule, LinearStartStop)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "linear",
    "initial": 5,
    "final": 3,
    "start": 2,
    "stop": 4
  })");
  schedule.configure(input);
  // |__           .
  // |  \          .
  // |   \         .
  // |    \____    .
  // |             .
  // +--+--+--+---
  // 0  2  4
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 5);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 5);
  EXPECT_DOUBLE_EQ(schedule.get_value(3), 4);
  EXPECT_DOUBLE_EQ(schedule.get_value(4), 3);
  EXPECT_DOUBLE_EQ(schedule.get_value(5), 3);
}

TEST(Schedule, LinearRepeat)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "linear",
    "initial": 3,
    "final": 5,
    "start": 2,
    "stop": 4,
    "repeat": true
  })");
  schedule.configure(input);
  // |
  // |  /|  /|  /|  /
  // | / | / | / | /
  // |/  |/  |/  |/
  // |
  // +---+---+---+-------
  //     2   4   6
  EXPECT_DOUBLE_EQ(schedule.get_value(1.0), 4.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1.5), 4.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(2.0), 3.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(2.5), 3.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(3.0), 4.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(3.5), 4.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(4.0), 3.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(4.5), 3.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(6.5), 3.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(7.5), 4.5);
}
