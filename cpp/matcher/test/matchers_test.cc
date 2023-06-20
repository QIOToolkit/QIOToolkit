// Copyright (c) Microsoft. All Rights Reserved.
#include "matcher/matchers.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

using ::matcher::Any;
using ::matcher::Each;
using ::matcher::EqualTo;
using ::matcher::GreaterEqual;
using ::matcher::GreaterThan;
using ::matcher::IsEmpty;
using ::matcher::LessEqual;
using ::matcher::LessThan;
using ::matcher::SizeIs;

TEST(Matchers, Any)
{
  Any().matches(false);
  Any().matches(3);
  Any().matches(3);
  Any().matches("hello");
}

TEST(Matchers, Integer)
{
  EXPECT_TRUE(EqualTo(3).matches(3));
  EXPECT_EQ(EqualTo(3).explain(3), "is equal to 3");
  EXPECT_FALSE(EqualTo(4).matches(3));
  EXPECT_EQ(EqualTo(4).explain(3), "must be equal to 4, found 3");
  EXPECT_FALSE(EqualTo(3).matches(4));
  EXPECT_EQ(EqualTo(3).explain(4), "must be equal to 3, found 4");

  EXPECT_TRUE(LessThan(4).matches(3));
  EXPECT_FALSE(LessThan(3).matches(3));
  EXPECT_FALSE(LessThan(3).matches(4));

  EXPECT_TRUE(LessEqual(4).matches(3));
  EXPECT_TRUE(LessEqual(4).matches(4));
  EXPECT_FALSE(LessEqual(3).matches(4));

  EXPECT_TRUE(GreaterThan(3).matches(4));
  EXPECT_FALSE(GreaterThan(4).matches(4));
  EXPECT_FALSE(GreaterThan(4).matches(3));

  EXPECT_TRUE(GreaterEqual(3).matches(4));
  EXPECT_TRUE(GreaterEqual(3).matches(3));
  EXPECT_FALSE(GreaterEqual(4).matches(3));

  EXPECT_FALSE(EqualTo(3.5).matches(3));
  EXPECT_TRUE(LessThan(3.5).matches(3));
  EXPECT_FALSE(LessThan(3.5).matches(4));
}

TEST(Matchers, Double)
{
  EXPECT_TRUE(EqualTo(0.3).matches(0.3));
  EXPECT_FALSE(EqualTo(0.4).matches(0.3));
  EXPECT_FALSE(EqualTo(0.3).matches(0.4));

  EXPECT_TRUE(LessThan(0.4).matches(0.3));
  EXPECT_FALSE(LessThan(0.3).matches(0.3));
  EXPECT_FALSE(LessThan(0.3).matches(0.4));

  EXPECT_TRUE(LessEqual(0.4).matches(0.3));
  EXPECT_TRUE(LessEqual(0.4).matches(0.4));
  EXPECT_FALSE(LessEqual(0.3).matches(0.4));

  EXPECT_TRUE(GreaterThan(0.3).matches(0.4));
  EXPECT_FALSE(GreaterThan(0.4).matches(0.4));
  EXPECT_FALSE(GreaterThan(0.4).matches(0.3));

  EXPECT_TRUE(GreaterEqual(0.3).matches(0.4));
  EXPECT_TRUE(GreaterEqual(0.3).matches(0.3));
  EXPECT_FALSE(GreaterEqual(0.4).matches(0.3));
}

TEST(Matchers, IntAndDouble)
{
  EXPECT_FALSE(EqualTo(1).matches(1.5));
  EXPECT_FALSE(EqualTo(1.5).matches(1));
}

TEST(Matchers, Vector)
{
  std::vector<int> vi0 = {};
  std::vector<int> vi3 = {1, 2, 3};
  std::vector<int> vi4 = {1, 2, 3, -4};

  auto size3 = SizeIs(EqualTo(3));
  EXPECT_TRUE(size3.matches(vi3));
  EXPECT_FALSE(size3.matches(vi4));

  auto size3plus = SizeIs(GreaterEqual(3));
  EXPECT_FALSE(size3plus.matches(vi0));
  EXPECT_TRUE(size3plus.matches(vi3));
  EXPECT_TRUE(size3plus.matches(vi4));

  auto empty = IsEmpty();
  EXPECT_TRUE(empty.matches(vi0));
  EXPECT_FALSE(empty.matches(vi3));
  EXPECT_EQ(empty.explain(vi0), "is empty");
  EXPECT_EQ(empty.explain(vi0), "is empty");
  EXPECT_EQ(empty.explain(vi3), "must be empty, actual size: 3");

  auto each_positive = Each(GreaterEqual(0));
  EXPECT_TRUE(each_positive.matches(vi0));
  EXPECT_EQ(each_positive.explain(vi0), "is empty (always true)");
  EXPECT_TRUE(each_positive.matches(vi3));
  EXPECT_EQ(each_positive.explain(vi3), "each element is greater-equal 0");
  EXPECT_FALSE(each_positive.matches(vi4));
  EXPECT_EQ(each_positive.explain(vi4),
            "each element must be greater-equal 0, found -4 at position 3");

  auto non_empty_and_positive =
      AllOf(SizeIs(GreaterThan(0)), Each(GreaterEqual(0)));
  EXPECT_FALSE(non_empty_and_positive.matches(vi0));
  EXPECT_EQ(non_empty_and_positive.explain(vi0),
            "size must be greater than 0, found 0");
  EXPECT_TRUE(non_empty_and_positive.matches(vi3));
  EXPECT_EQ(non_empty_and_positive.explain(vi3),
            "size is greater than 0 and each element is greater-equal 0");
  EXPECT_FALSE(non_empty_and_positive.matches(vi4));
  EXPECT_EQ(non_empty_and_positive.explain(vi4),
            "each element must be greater-equal 0, found -4 at position 3");
}
