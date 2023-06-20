// Copyright (c) Microsoft. All Rights Reserved.
#include "solver/test/test_model.h"

double TestModel::kInitValue = 42;
double TestModel::kBaseRandom = 100;
double TestModel::kBaseDiff = 5;
double TestModel::kBaseUniform = 10;

double TestModel::calculate_cost(const ValueState<double>& state) const
{
  return std::abs(state.value - kInitValue);
}

double TestModel::calculate_cost_difference(
    const ValueState<double>& state, const ValueState<double>& transition) const
{
  return calculate_cost(transition) - calculate_cost(state);
}

ValueState<double> TestModel::get_random_state(utils::RandomGenerator& rng) const
{
  return ValueState<double>(rng.uniform() * kBaseRandom);
}

ValueState<double> TestModel::get_random_transition(
    const ValueState<double>& state, utils::RandomGenerator& rng) const
{
  return ValueState<double>(state.value + rng.uniform() * kBaseUniform -
                            kBaseDiff);
}

void TestModel::apply_transition(const ValueState<double>& transition,
                                 ValueState<double>& state) const
{
  state = transition;
}

size_t TestModel::get_sweep_size() const { return 10; }
