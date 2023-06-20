// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/utils.h"

#include <cmath>

#include "gtest/gtest.h"

TEST(Utils, PopLargets)
{
  std::multiset<double> sorted;
  sorted.insert(5);
  sorted.insert(1);
  sorted.insert(-2);
  sorted.insert(10);
  double value = utils::pop_largest(sorted);
  EXPECT_DOUBLE_EQ(10, value);
  EXPECT_EQ(3, sorted.size());
  value = utils::pop_largest(sorted);
  EXPECT_DOUBLE_EQ(5, value);
  EXPECT_EQ(2, sorted.size());
}


TEST(Utils, GetCombinedHash)
{
  // Tests to check if result will be reproduced.
  EXPECT_EQ(utils::get_combined_hash(5, 6), utils::get_combined_hash(5, 6));
  EXPECT_EQ(utils::get_combined_hash(5, 6, 6.1),
            utils::get_combined_hash(5, 6, 6.1));

  // Due to possibility of hash collisions,
  // some of following tests may fail with change of implementation.
  // Yet probability of collisions should be low,
  // so only a major issue can cause fail of many of tests.
  EXPECT_NE(utils::get_combined_hash(5, 6.0), utils::get_combined_hash(5, 6.1));
  EXPECT_NE(utils::get_combined_hash((int)-5, 6.0),
            utils::get_combined_hash((int)5, 6.0));
  EXPECT_NE(utils::get_combined_hash(5, 6.0, 1),
            utils::get_combined_hash(5, 6.0, 2));
  EXPECT_NE(utils::get_combined_hash(-1, 234), utils::get_combined_hash(-1, 222));
  EXPECT_NE(utils::get_combined_hash(-2342345, 6.0),
            utils::get_combined_hash(-2342346, 6.0));
  EXPECT_NE(utils::get_combined_hash(5, 1.745475),
            utils::get_combined_hash(5, 1.745476));
  EXPECT_NE(utils::get_combined_hash(1, 2, 2), utils::get_combined_hash(1,2,1));
  EXPECT_NE(utils::get_combined_hash(-1.1, 6.0),
            utils::get_combined_hash(1.1, 6.1));
  EXPECT_NE(utils::get_combined_hash(5, 6.0), utils::get_combined_hash(-5, -6.0));
  EXPECT_NE(utils::get_combined_hash(1, 2), utils::get_combined_hash(-1, -2));
  EXPECT_NE(utils::get_combined_hash(0, 0), utils::get_combined_hash(0, 1));
}
