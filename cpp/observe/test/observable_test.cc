
#include "observe/observable.h"

#include <string>

#include "gtest/gtest.h"

using ::observe::Average;
using ::observe::Average2;
using ::observe::Batched;
using ::observe::BinWidthDistribution;
using ::observe::Constant;
using ::observe::MinMax;
using ::observe::Observable;
using ::observe::RangedDistribution;
using ::observe::Windowed;

TEST(Observable, Constant)
{
  Constant constant;
  EXPECT_EQ(constant.render().to_string(), R"(nan)");
  constant.record(42);
  EXPECT_EQ(constant.render().to_string(), R"(42.000000)");
  constant.record(-13);
  EXPECT_EQ(constant.render().to_string(), R"(-13.000000)");
  constant.reset();
  EXPECT_EQ(constant.render().to_string(), R"(nan)");
}

TEST(Observable, Average)
{
  Average average;
  EXPECT_EQ(average.render().to_string(), "nan");
  average.record(10);
  EXPECT_EQ(average.render().to_string(), "10.000000");
  average.record(20);
  EXPECT_EQ(average.render().to_string(), "15.000000");
  average.record(30);
  EXPECT_EQ(average.render().to_string(), "20.000000");
  average.reset();
  EXPECT_EQ(average.render().to_string(), "nan");
}

TEST(Observable, MinMax)
{
  MinMax min_max;
  EXPECT_EQ(min_max.render().to_string(), R"({
  "count": 0.000000,
  "max": nan,
  "mean": nan,
  "min": nan
})");
  min_max.record(20);
  EXPECT_EQ(min_max.render().to_string(), R"({
  "count": 1.000000,
  "max": 20.000000,
  "mean": 20.000000,
  "min": 20.000000
})");
  min_max.record(10);
  EXPECT_EQ(min_max.render().to_string(), R"({
  "count": 2.000000,
  "max": 20.000000,
  "mean": 15.000000,
  "min": 10.000000
})");
  min_max.record(30);
  EXPECT_EQ(min_max.render().to_string(), R"({
  "count": 3.000000,
  "max": 30.000000,
  "mean": 20.000000,
  "min": 10.000000
})");
  min_max.reset();
  EXPECT_EQ(min_max.render().to_string(), R"({
  "count": 0.000000,
  "max": nan,
  "mean": nan,
  "min": nan
})");
}

TEST(Observable, Average2)
{
  Average2 average2;
  EXPECT_EQ(average2.render().to_string(), R"({
  "count": 0.000000,
  "dev": nan,
  "max": nan,
  "mean": nan,
  "min": nan
})");
  average2.record(10);
  EXPECT_EQ(average2.render().to_string(), R"({
  "count": 1.000000,
  "dev": 0.000000,
  "max": 10.000000,
  "mean": 10.000000,
  "min": 10.000000
})");
  average2.record(20);
  EXPECT_EQ(average2.render().to_string(), R"({
  "count": 2.000000,
  "dev": 5.000000,
  "max": 20.000000,
  "mean": 15.000000,
  "min": 10.000000
})");
  average2.record(30);
  EXPECT_EQ(average2.render().to_string(), R"({
  "count": 3.000000,
  "dev": 8.164966,
  "max": 30.000000,
  "mean": 20.000000,
  "min": 10.000000
})");
  average2.reset();
  EXPECT_EQ(average2.render().to_string(), R"({
  "count": 0.000000,
  "dev": nan,
  "max": nan,
  "mean": nan,
  "min": nan
})");
}

TEST(Observable, RangedDistribution)
{
  RangedDistribution dist(1, 5, 4);
  EXPECT_EQ(dist.render().to_string(), R"({
  "bins": [
    {"count": 0.000000, "value": 1.500000},
    {"count": 0.000000, "value": 2.500000},
    {"count": 0.000000, "value": 3.500000},
    {"count": 0.000000, "value": 4.500000}
  ],
  "count": 0.000000,
  "dev": nan,
  "max": nan,
  "mean": nan,
  "min": nan
})");

  dist.record(4);
  dist.record(2);
  dist.record(0);

  EXPECT_EQ(dist.render().to_string(), R"({
  "bins": [
    {"count": 0.000000, "value": 1.500000},
    {"count": 1.000000, "value": 2.500000},
    {"count": 0.000000, "value": 3.500000},
    {"count": 1.000000, "value": 4.500000}
  ],
  "count": 3.000000,
  "dev": 1.632993,
  "max": 4.000000,
  "mean": 2.000000,
  "min": 0.000000
})");
}

TEST(Observable, BinWidthDistribution)
{
  BinWidthDistribution dist(0.5);
  EXPECT_EQ(dist.render().to_string(), R"({
  "count": 0.000000,
  "dev": nan,
  "max": nan,
  "mean": nan,
  "min": nan
})");

  dist.record(4);
  dist.record(2);
  dist.record(0);

  EXPECT_EQ(dist.render().to_string(), R"({
  "bins": [
    {"count": 1.000000, "value": 0.000000},
    {"count": 1.000000, "value": 2.000000},
    {"count": 1.000000, "value": 4.000000}
  ],
  "count": 3.000000,
  "dev": 1.632993,
  "max": 4.000000,
  "mean": 2.000000,
  "min": 0.000000
})");
}

TEST(Observable, Windowed)
{
  Windowed windowed(std::unique_ptr<Observable>(new Average()), 2);
  EXPECT_EQ(windowed.render().to_string(), "nan");
  windowed.record(10);
  EXPECT_EQ(windowed.render().to_string(), "10.000000");
  windowed.record(20);
  EXPECT_EQ(windowed.render().to_string(), "15.000000");
  windowed.record(30);  // 10 is dropped from the window
  EXPECT_EQ(windowed.render().to_string(), "25.000000");
  windowed.reset();
  EXPECT_EQ(windowed.render().to_string(), "nan");
}

TEST(Observable, WindowedHistogram)
{
  Windowed dist(std::unique_ptr<Observable>(new BinWidthDistribution(0.5)), 2);
  EXPECT_EQ(dist.render().to_string(), R"({
  "count": 0.000000,
  "dev": nan,
  "max": nan,
  "mean": nan,
  "min": nan
})");

  dist.record(4);
  dist.record(2);
  EXPECT_EQ(dist.render().to_string(), R"({
  "bins": [
    {"count": 1.000000, "value": 2.000000},
    {"count": 1.000000, "value": 4.000000}
  ],
  "count": 2.000000,
  "dev": 1.000000,
  "max": 4.000000,
  "mean": 3.000000,
  "min": 2.000000
})");

  dist.record(0);  // 4 is dropped form the window
  EXPECT_EQ(dist.render().to_string(), R"({
  "bins": [
    {"count": 1.000000, "value": 0.000000},
    {"count": 1.000000, "value": 2.000000}
  ],
  "count": 2.000000,
  "dev": 1.000000,
  "max": 2.000000,
  "mean": 1.000000,
  "min": 0.000000
})");
}

TEST(Observable, Batched)
{
  Batched batched(std::unique_ptr<Observable>(new Average()), 2);
  // first batch
  batched.record(10);
  batched.record(20);
  // second batch
  batched.record(30);
  batched.record(40);
  EXPECT_EQ(batched.render().to_string(), R"([15.000000,35.000000])");
}
