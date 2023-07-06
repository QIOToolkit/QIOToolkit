
#include "markov/model.h"

#include <bitset>
#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "gtest/gtest.h"

using ::utils::RandomGenerator;
using ::utils::Structure;
using ::markov::Model;
using ::markov::State;
using ::markov::Transition;

class TestState : public State
{
 public:
  TestState(int value_) : value(value_) {}
  void configure(const utils::Json& json) override
  {
    utils::set_from_json(value, json);
  }
  Structure render() const override
  {
    Structure s;
    s = value;
    return s;
  }
  void copy_state_only(const TestState& other) { value = other.value; }
  static size_t memory_estimate() { return sizeof(TestState); }
  static size_t state_only_memory_estimate() { return memory_estimate(); }
  int value;
};

class TestTransition : public Transition
{
 public:
  TestTransition(int mask_) : mask(mask_) {}
  int mask;
};

class TestModel : public Model<TestState, TestTransition>
{
 public:
  std::string get_identifier() const override { return "test_model"; }
  std::string get_version() const override { return "0.0"; }

  double calculate_cost(const TestState& state) const override
  {
    return (int)std::bitset<8>(state.value).count();
  }

  double calculate_cost_difference(
      const TestState& state, const TestTransition& transition) const override
  {
    return int(std::bitset<8>(state.value ^ transition.mask).count()) -
           int(std::bitset<8>(state.value).count());
  }

  TestState get_random_state(utils::RandomGenerator& rng) const override
  {
    return int(rng.uniform() * 0xff);
  }

  TestTransition get_random_transition(
      const TestState&, utils::RandomGenerator& rng) const override
  {
    return int(rng.uniform() * 0xff);
  }

  void apply_transition(const TestTransition& transition,
                        TestState& state) const override
  {
    state.value ^= transition.mask;
  }

  size_t state_memory_estimate() const override
  {
    return TestState::memory_estimate();
  }

  size_t state_only_memory_estimate() const override
  {
    return TestState::state_only_memory_estimate();
  }

  double get_const_cost() const override { return 0; }

  bool is_empty() const override { return false; }
};

TEST(Model, CalculatesCost)
{
  TestState state = 32 + 8 + 2;
  TestModel model;
  EXPECT_EQ(model.calculate_cost(state), 3);
}

TEST(Model, CalculatesCostDifference)
{
  TestState state = 0x55;
  TestTransition transition = 0x4A;
  TestModel model;
  EXPECT_EQ(model.calculate_cost_difference(state, transition), 2 - 1);
}

TEST(Model, RandomState)
{
  utils::Twister rng;
  TestModel model;
  TestState state = model.get_random_state(rng);
  EXPECT_TRUE(state.value >= 0 && state.value < 0xff);
}

TEST(Model, RandomTransition)
{
  utils::Twister rng;
  TestModel model;
  TestState state = 42;
  TestTransition transition = model.get_random_transition(state, rng);
  EXPECT_TRUE(transition.mask >= 0 && transition.mask < 0xff);
}

TEST(Model, AppliesTransition)
{
  TestModel model;
  TestState state = 0x55;
  TestTransition transition = 0x4A;
  model.apply_transition(transition, state);
  EXPECT_EQ(state.value, 0x1F);
}

TEST(Model, Debugs)
{
  TestModel model;
  std::stringstream s;
  s << model;
  EXPECT_EQ(s.str(), "<TestModel>");
}

TEST(Model, Metropolis)
{
  double T = 0.5;
  utils::Twister rng;
  TestModel model;
  auto state = model.get_random_state(rng);
  double cost = model.calculate_cost(state);
  double min = cost;
  for (int i = 0; i < 100; i++)
  {
    auto transition = model.get_random_transition(state, rng);
    double diff = model.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      model.apply_transition(transition, state);
      cost += diff;
    }
    min = std::min(min, cost);
  }
  EXPECT_EQ(min, 0);
}
