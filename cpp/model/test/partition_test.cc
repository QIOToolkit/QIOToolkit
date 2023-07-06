
#include "model/partition.h"

#include "gtest/gtest.h"

using ::model::Partition;

TEST(Partition, Create)
{
  utils::Twister rng;
  rng.seed(42);
  auto partition = Partition::random(10, 4, rng);
  // std::shuffle implementation depend on compiler
#ifdef _MSC_VER
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [1,2,6,8], "second": [0,3,4,5,7,9]})");
#else
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [0,3,7,8], "second": [1,2,4,5,6,9]})");
#endif
}

TEST(Partition, Query)
{
  utils::Twister rng;
  rng.seed(42);
  auto partition = Partition::random(10, 4, rng);
  // std::shuffle implementation depend on compiler
#ifdef _MSC_VER
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [1,2,6,8], "second": [0,3,4,5,7,9]})");

  EXPECT_FALSE(partition.in_first(0));
  EXPECT_FALSE(partition.in_first(3));
  EXPECT_FALSE(partition.in_first(7));
  EXPECT_TRUE(partition.in_first(8));
  EXPECT_TRUE(partition.in_first(1));
  EXPECT_TRUE(partition.in_first(2));
  EXPECT_FALSE(partition.in_first(4));
  EXPECT_FALSE(partition.in_first(5));
  EXPECT_TRUE(partition.in_first(6));
  EXPECT_FALSE(partition.in_first(9));

#else
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [0,3,7,8], "second": [1,2,4,5,6,9]})");

  EXPECT_TRUE(partition.in_first(0));
  EXPECT_TRUE(partition.in_first(3));
  EXPECT_TRUE(partition.in_first(7));
  EXPECT_TRUE(partition.in_first(8));
  EXPECT_FALSE(partition.in_first(1));
  EXPECT_FALSE(partition.in_first(2));
  EXPECT_FALSE(partition.in_first(4));
  EXPECT_FALSE(partition.in_first(5));
  EXPECT_FALSE(partition.in_first(6));
  EXPECT_FALSE(partition.in_first(9));
#endif
}

TEST(Partition, Swap)
{
  utils::Twister rng;
  rng.seed(42);
  auto partition = Partition::random(10, 4, rng);
  // std::shuffle implementation depend on compiler
#ifdef _MSC_VER
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [1,2,6,8], "second": [0,3,4,5,7,9]})");
#else
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [0,3,7,8], "second": [1,2,4,5,6,9]})");

#endif
  partition.swap_indices(2, 3);
#ifdef _MSC_VER
  // result of swap index 2 in "first" (#6) with index 3 in "second" (#5)
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [1,2,5,8], "second": [0,3,4,6,7,9]})");

  EXPECT_FALSE(partition.in_first(0));
  EXPECT_FALSE(partition.in_first(3));
  EXPECT_TRUE(partition.in_first(5));
  EXPECT_TRUE(partition.in_first(8));
  EXPECT_TRUE(partition.in_first(1));
  EXPECT_TRUE(partition.in_first(2));
  EXPECT_FALSE(partition.in_first(4));
  EXPECT_FALSE(partition.in_first(6));
  EXPECT_FALSE(partition.in_first(7));
  EXPECT_FALSE(partition.in_first(9));
#else
  // result of swap index 2 in "first" (#7) with index 3 in "second" (#5)
  EXPECT_EQ(partition.render().to_string(),
            R"({"first": [0,3,5,8], "second": [1,2,4,6,7,9]})");

  EXPECT_TRUE(partition.in_first(0));
  EXPECT_TRUE(partition.in_first(3));
  EXPECT_TRUE(partition.in_first(5));
  EXPECT_TRUE(partition.in_first(8));
  EXPECT_FALSE(partition.in_first(1));
  EXPECT_FALSE(partition.in_first(2));
  EXPECT_FALSE(partition.in_first(4));
  EXPECT_FALSE(partition.in_first(6));
  EXPECT_FALSE(partition.in_first(7));
  EXPECT_FALSE(partition.in_first(9));

#endif
}
