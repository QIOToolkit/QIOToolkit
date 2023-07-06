
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

TEST(Schedule, Geometric)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 3,
    "final": 6
  })");
  schedule.configure(input);
  //    |     _____
  //    |    /
  //    |   .
  // ___|_--
  //    |
  // ---+-----+-----
  //    0     1
  EXPECT_DOUBLE_EQ(schedule.get_value(-1), 3);
  EXPECT_DOUBLE_EQ(schedule.get_value(0), 3);
  EXPECT_DOUBLE_EQ(schedule.get_value(0.5), 3 * pow(2, 0.5));
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 6);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 6);
}

TEST(Schedule, GeometricStartStop)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 6,
    "final": 3,
    "start": 2,
    "stop": 4
  })");
  schedule.configure(input);
  // |___
  // |   |
  // |    .
  // |     --_____
  // |
  // +---+---+-----
  // 0   2   4
  EXPECT_DOUBLE_EQ(schedule.get_value(1), 6);
  EXPECT_DOUBLE_EQ(schedule.get_value(2), 6);
  EXPECT_DOUBLE_EQ(schedule.get_value(3), 6 * pow(2, -0.5));
  EXPECT_DOUBLE_EQ(schedule.get_value(4), 3);
  EXPECT_DOUBLE_EQ(schedule.get_value(5), 3);
}

TEST(Schedule, GeometricRepeat)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 6,
    "final": 3,
    "start": 2,
    "stop": 4,
    "repeat": true
  })");
  schedule.configure(input);
  // |
  // |\    |\    |\    |
  // | .   | .   | .   |
  // |  --_|  --_|  --_|
  // |
  // +-----+-----+-----+-------
  //       2     4     6
  EXPECT_DOUBLE_EQ(schedule.get_value(1.0), 6 * pow(2, -0.50));
  EXPECT_DOUBLE_EQ(schedule.get_value(1.5), 6 * pow(2, -0.75));
  EXPECT_DOUBLE_EQ(schedule.get_value(2.0), 6);
  EXPECT_DOUBLE_EQ(schedule.get_value(2.5), 6 * pow(2, -0.25));
  EXPECT_DOUBLE_EQ(schedule.get_value(3.0), 6 * pow(2, -0.50));
  EXPECT_DOUBLE_EQ(schedule.get_value(3.5), 6 * pow(2, -0.75));
  EXPECT_DOUBLE_EQ(schedule.get_value(4.0), 6);
  EXPECT_DOUBLE_EQ(schedule.get_value(4.5), 6 * pow(2, -0.25));
  EXPECT_DOUBLE_EQ(schedule.get_value(7.5), 6 * pow(2, -0.75));
}

TEST(Schedule, ExponentialWithCount100)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 10,
    "final": 1,
    "count": 100
  })");
  schedule.configure(input);
  // 10 |
  //    ||
  //    ||
  //    |.
  // 1  | -.__
  // ---+-----+-----
  //    0    100

  std::vector<double> values = schedule.get_discretized_values();

  EXPECT_NEAR(values[0], 10, 0.00000000000001);
  EXPECT_NEAR(values[1], 9.7700995729922528, 0.00000000000001);
  EXPECT_NEAR(values[5], 8.9021508544503831, 0.00000000000001);
  EXPECT_NEAR(values[12], 7.5646332755462842, 0.00000000000001);
  EXPECT_NEAR(values[24], 5.7223676593502102, 0.00000000000001);
  EXPECT_NEAR(values[37], 4.2292428743894899, 0.00000000000001);
  EXPECT_NEAR(values[45], 3.5111917342151227, 0.00000000000001);
  EXPECT_NEAR(values[52], 2.9836472402833309, 0.00000000000001);
  EXPECT_NEAR(values[67], 2.1049041445120129, 0.00000000000001);
  EXPECT_NEAR(values[70], 1.9630406500402637, 0.00000000000001);
  EXPECT_NEAR(values[82], 1.4849682622544584, 0.00000000000001);
  EXPECT_NEAR(values[95], 1.0974987654930506, 0.00000000000001);
  EXPECT_NEAR(values[99], 0.9999999999999947, 0.00000000000001);
}

TEST(Schedule, ExponentialComparison)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 10,
    "final": 1,
    "count": 100
  })");
  schedule.configure(input);
  std::vector<double> values = schedule.get_discretized_values();

  Schedule schedule2;
  auto input2 = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 10,
    "final": 1,
    "start": 0,
    "stop": 99
  })");
  schedule2.configure(input2);
  std::vector<double> values2 = schedule2.get_discretized_values();

  EXPECT_EQ(values.size(), values2.size());
  for (size_t i = 0; i < values.size(); i++)
  {
    EXPECT_EQ(values[i], values2[i]);
  }
}

TEST(Schedule, ExponentialWithCount10000)
{
  Schedule schedule;
  auto input = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 10,
    "final": 1,
    "count": 10000
  })");
  schedule.configure(input);
  // 10 |
  //    ||
  //    ||
  //    |.
  // 1  | -.__
  // ---+-----+-----
  //    0   10000

  std::vector<double> values = schedule.get_discretized_values();

  EXPECT_NEAR(values[0], 10, 0.000000000001);
  EXPECT_NEAR(values[500], 8.9124067626100203, 0.000000000001);
  EXPECT_NEAR(values[1000], 7.9430994302216824, 0.000000000001);
  EXPECT_NEAR(values[1500], 7.0792133077991526, 0.000000000001);
  EXPECT_NEAR(values[2000], 6.3092828558388012, 0.000000000001);
  EXPECT_NEAR(values[2500], 5.6230895191597199, 0.000000000001);
  EXPECT_NEAR(values[3000], 5.0115261057320613, 0.000000000001);
  EXPECT_NEAR(values[3500], 4.4664759155723086, 0.000000000001);
  EXPECT_NEAR(values[4000], 3.9807050154981427, 0.000000000001);
  EXPECT_NEAR(values[4500], 3.5477662300081274, 0.000000000001);
  EXPECT_NEAR(values[5000], 3.1619135740483895, 0.000000000001);
  EXPECT_NEAR(values[5500], 2.8180259920137285, 0.000000000001);
  EXPECT_NEAR(values[6000], 2.5115393908433963, 0.000000000001);
  EXPECT_NEAR(values[6500], 2.2383860651514138, 0.000000000001);
  EXPECT_NEAR(values[7000], 1.9949407104387495, 0.000000000001);
  EXPECT_NEAR(values[7500], 1.7779723078720351, 0.000000000001);
  EXPECT_NEAR(values[8000], 1.5846012420412070, 0.000000000001);
  EXPECT_NEAR(values[8500], 1.4122610825608288, 0.000000000001);
  EXPECT_NEAR(values[9000], 1.2586645222786079, 0.000000000001);
  EXPECT_NEAR(values[9500], 1.1217730200213176, 0.000000000001);
  EXPECT_NEAR(values[9999], 0.9999999999998409, 0.000000000001);
}

TEST(Schedule, GeometricPositiveInput)
{
  Schedule schedule;
  auto input1 = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 0,
    "final": 3,
    "start": 2,
    "stop": 4,
    "repeat": true
  })");
  EXPECT_THROW_MESSAGE(
      schedule.configure(input1), utils::InitialConfigException,
      "unable to initialize schedule:\n  Linear: parameter `type`: must be "
      "equal to 'linear', found 'geometric'\n  Geometric: initial must be "
      "positive for geometric\n  Segments: parameter `type`: must be equal to "
      "'segments', found 'geometric'\n  Constant: parameter `type`: must be "
      "equal to 'constant', found 'geometric'");

  auto input2 = utils::json_from_string(R"({
    "type": "geometric",
    "initial": 6,
    "final": 0,
    "start": 2,
    "stop": 4,
    "repeat": true
  })");
  EXPECT_THROW_MESSAGE(
      schedule.configure(input2), utils::InitialConfigException,
      "unable to initialize schedule:\n  Linear: parameter `type`: must be "
      "equal to 'linear', found 'geometric'\n  Geometric: final must be "
      "positive for geometric\n  Segments: parameter `type`: must be equal to "
      "'segments', found 'geometric'\n  Constant: parameter `type`: must be "
      "equal to 'constant', found 'geometric'");
}
