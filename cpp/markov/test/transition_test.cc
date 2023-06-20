// Copyright (c) Microsoft. All Rights Reserved.
#include "markov/transition.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "gtest/gtest.h"

using ::markov::Transition;

class TestTransition : public Transition
{
 public:
  TestTransition(int mask) : mask(mask) {}
  utils::Structure get_status() const override
  {
    std::stringstream s;
    s << "0x" << std::hex << mask;
    utils::Structure structure;
    structure = s.str();
    return structure;
  }

 private:
  int mask;
};

TEST(Transition, Debugs)
{
  TestTransition transition(0x55);
  std::stringstream s;
  s << transition;
  EXPECT_EQ(s.str(), "<TestTransition: 0x55>");
}
