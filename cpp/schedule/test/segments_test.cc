
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

TEST(Schedule, SegmentsFromArray)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"(
    [1,3,2]
  )");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(-1.), 1.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.0), 1.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.5), 2.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1.0), 3.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1.5), 2.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(3.0), 2.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(4.0), 2.0);
  EXPECT_THAT(schedule.get_discretized_values(), ElementsAre(1, 3, 2));
}

TEST(Schedule, SegmentsFromConstants)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "segments",
    "segments": [1,3,2]
  })");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(-1.), 1.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.0), 1.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.5), 2.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1.0), 3.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(1.5), 2.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(3.0), 2.0);
  EXPECT_DOUBLE_EQ(schedule.get_value(4.0), 2.0);
  EXPECT_THAT(schedule.get_discretized_values(), ElementsAre(1, 3, 2));
}

TEST(Schedule, ConstantLinearConstant)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "segments",
    "segments": [
      {
        "type": "constant",
        "value": 0.8,
        "start": 0,
        "stop": 200
      },
      {
        "type": "linear",
        "initial": 0.8,
        "final": 0.2,
        "start": 200,
        "stop": 800
      },
      {
        "type": "constant",
        "value": 0.2,
        "start": 800,
        "stop": 1000
      }
    ]
  })");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(0), 0.8);
  EXPECT_DOUBLE_EQ(schedule.get_value(100), 0.8);
  EXPECT_DOUBLE_EQ(schedule.get_value(200), 0.8);
  EXPECT_DOUBLE_EQ(schedule.get_value(300), 0.7);
  EXPECT_DOUBLE_EQ(schedule.get_value(400), 0.6);
  EXPECT_DOUBLE_EQ(schedule.get_value(500), 0.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(600), 0.4);
  EXPECT_DOUBLE_EQ(schedule.get_value(700), 0.3);
  EXPECT_DOUBLE_EQ(schedule.get_value(800), 0.2);
  EXPECT_DOUBLE_EQ(schedule.get_value(900), 0.2);
  EXPECT_DOUBLE_EQ(schedule.get_value(900), 0.2);
  EXPECT_THAT(schedule.get_discretized_values(6),
              ElementsAre(DoubleEq(0.8), DoubleEq(0.8), DoubleEq(0.6),
                          DoubleEq(0.4), DoubleEq(0.2), DoubleEq(0.2)));
}

TEST(Schedule, ConstantLinearConstantWithCounts)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "segments",
    "segments": [
      {
        "type": "constant",
        "value": 0.8,
        "count": 200
      },
      {
        "type": "linear",
        "initial": 0.8,
        "final": 0.2,
        "count": 600
      },
      {
        "type": "constant",
        "value": 0.2,
        "count": 200
      }
    ]
  })");
  schedule.configure(input);
  EXPECT_DOUBLE_EQ(schedule.get_value(0), 0.8);
  EXPECT_DOUBLE_EQ(schedule.get_value(100), 0.8);
  EXPECT_DOUBLE_EQ(schedule.get_value(200), 0.8);
  EXPECT_DOUBLE_EQ(schedule.get_value(300), 0.7);
  EXPECT_DOUBLE_EQ(schedule.get_value(400), 0.6);
  EXPECT_DOUBLE_EQ(schedule.get_value(500), 0.5);
  EXPECT_DOUBLE_EQ(schedule.get_value(600), 0.4);
  EXPECT_DOUBLE_EQ(schedule.get_value(700), 0.3);
  EXPECT_DOUBLE_EQ(schedule.get_value(800), 0.2);
  EXPECT_DOUBLE_EQ(schedule.get_value(900), 0.2);
  EXPECT_DOUBLE_EQ(schedule.get_value(900), 0.2);
  EXPECT_THAT(schedule.get_discretized_values(6),
              ElementsAre(DoubleEq(0.8), DoubleEq(0.8), DoubleEq(0.6),
                          DoubleEq(0.4), DoubleEq(0.2), DoubleEq(0.2)));
}

TEST(Schedule, Repetitions)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "segments",
    "segments": [
      {
        "type": "constant",
        "value": 1,
        "count": 1
      },
      {
        "type": "constant",
        "value": 2,
        "count": 2
      },
      {
        "type": "constant",
        "value": 3,
        "count": 3
      }
    ]
  })");
  schedule.configure(input);
  EXPECT_THAT(schedule.get_discretized_values(), ElementsAre(1, 2, 2, 3, 3, 3));
}
