// Copyright (c) Microsoft. All Rights Reserved.
#include "model/permutation.h"

#include <cmath>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

using ::model::Permutation;

TEST(Permutation, Repr)
{
  Permutation p(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
}

TEST(Permutation, MoveNode)
{
  Permutation p(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.move_node(1, 1);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.move_node(1, 3);
  EXPECT_EQ(p.render().to_string(), "[0,2,3,1,4]");

  p = Permutation(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.move_node(4, 0);
  EXPECT_EQ(p.render().to_string(), "[4,1,2,3,0]");
}

TEST(Permutation, SwapNodes)
{
  Permutation p(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.swap_nodes(1, 3);
  EXPECT_EQ(p.render().to_string(), "[0,3,2,1,4]");
  p.swap_nodes(4, 0);
  EXPECT_EQ(p.render().to_string(), "[4,3,2,1,0]");
}

TEST(Permutation, SwapEdges)
{
  Permutation p(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.swap_edges(0, 3);
  EXPECT_EQ(p.render().to_string(), "[3,2,1,0,4]");

  p = Permutation(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.swap_edges(2, 0);
  EXPECT_EQ(p.render().to_string(), "[2,1,0,4,3]");

  p = Permutation(5);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4]");
  p.swap_edges(4, 3);
  EXPECT_EQ(p.render().to_string(), "[2,1,0,4,3]");

  p = Permutation(6);
  EXPECT_EQ(p.render().to_string(), "[0,1,2,3,4,5]");
  p.swap_edges(4, 3);
  EXPECT_EQ(p.render().to_string(), "[1,0,5,4,3,2]");
}
