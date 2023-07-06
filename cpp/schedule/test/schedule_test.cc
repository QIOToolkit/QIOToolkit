
#include "schedule/schedule.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::utils::Json;
using ::schedule::Schedule;
using testing::DoubleEq;
using testing::ElementsAre;

TEST(Schedule, Linear)
{
  Schedule schedule;
  schedule.configure(utils::json_from_string(R"({
    "type": "linear",
    "initial": 3,
    "final": 0.5,
    "count": 6
  })"));
  EXPECT_DOUBLE_EQ(schedule.get_value(0.0), 3.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.2), 2.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.4), 2.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.6), 1.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.8), 1.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1.0), 0.5);
  EXPECT_THAT(schedule.get_discretized_values(),
              ElementsAre(DoubleEq(3), DoubleEq(2.5), DoubleEq(2),
                          DoubleEq(1.5), DoubleEq(1), DoubleEq(0.5)));
}

TEST(Schedule, Geometric)
{
  Schedule schedule;
  schedule.configure(utils::json_from_string(R"({
    "type": "geometric",
    "initial": 16,
    "final": 2,
    "count": 4
  })"));
  EXPECT_DOUBLE_EQ(schedule.get_value(0 / 3.), 16.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1 / 3.), 8.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(2 / 3.), 4.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(3 / 3.), 2.0);
  EXPECT_THAT(schedule.get_discretized_values(),
              ElementsAre(DoubleEq(16), DoubleEq(8), DoubleEq(4), DoubleEq(2)));
}

TEST(Schedule, FromArray)
{
  Schedule schedule;
  schedule.configure(utils::json_from_string("[1,2,3]"));
  EXPECT_DOUBLE_EQ(schedule.get_value(0), 1.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 2.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 3.0);
  EXPECT_THAT(schedule.get_discretized_values(),
              ElementsAre(DoubleEq(1), DoubleEq(2), DoubleEq(3)));
}
