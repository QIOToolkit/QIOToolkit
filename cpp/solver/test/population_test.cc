// Copyright (c) Microsoft. All Rights Reserved.
#include "solver/population.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "utils/random_generator.h"
#include "gtest/gtest.h"
#include "markov/metropolis.h"
#include "solver/test/test_model.h"

using ::utils::Structure;
using ::markov::Metropolis;
using ::markov::State;
using ::solver::Population;

class StringPopulationTest : public testing::Test
{
 public:
  typedef std::pair<std::string, int> Citizen;

  void create_population(std::vector<Citizen> initial)
  {
    for (size_t i = 0; i < initial.size(); i++)
    {
      population.insert(initial[i].first);
      population[i].set_count(initial[i].second);
    }
  }

  std::vector<Citizen> as_vector()
  {
    std::vector<Citizen> vec;
    for (size_t i = 0; i < population.size(); i++)
    {
      vec.push_back({*population[i], population[i].get_count()});
    }
    return vec;
  }

  void expect_resampled(std::vector<Citizen> expected)
  {
    population.resample();
    auto actual = as_vector();
    // NOTE: as-is, these tests check for the actual ordering produced
    // by the resampling logic. We can instead apply sort to both sides
    // to make the test ordering-agnostic:
    // std::sort(actual.begin(), actual.end());
    // std::sort(expected.begin(), expected.end());
    EXPECT_EQ(as_vector(), expected);
  }

  Population<std::string> population;
};

TEST_F(StringPopulationTest, Unchanged)
{
  create_population({{"foo", 1},  //
                     {"bar", 1},  //
                     {"baz", 1},  //
                     {"baaz", 1}});
  expect_resampled({{"foo", 1},  //
                    {"bar", 1},  //
                    {"baz", 1},  //
                    {"baaz", 1}});
}

TEST_F(StringPopulationTest, RemoveAtBegin)
{
  create_population({{"foo", 0},  // foo will be removed
                     {"bar", 1},  //
                     {"baz", 1},  //
                     {"baaz", 1}});
  expect_resampled({{"bar", 1},  //
                    {"baz", 1},  //
                    {"baaz", 1}});
}

TEST_F(StringPopulationTest, RemoveInMiddle)
{
  create_population({{"foo", 1},  //
                     {"bar", 0},  // bar will be removed
                     {"baz", 1},  //
                     {"baaz", 1}});
  expect_resampled({{"foo", 1},  //
                    {"baz", 1},  //
                    {"baaz", 1}});
}

TEST_F(StringPopulationTest, RemoveAtPopSize)
{
  create_population({{"foo", 1},  //
                     {"bar", 1},  //
                     {"baz", 0},  // baz will be removed
                     {"baaz", 1}});
  expect_resampled({{"foo", 1},  //
                    {"bar", 1},  //
                    {"baaz", 1}});
}

TEST_F(StringPopulationTest, RemoveAtEnd)
{
  create_population({{"foo", 1},     //
                     {"bar", 1},     //
                     {"baz", 1},     //
                     {"baaz", 0}});  // baaz will be removed
  expect_resampled({{"foo", 1},      //
                    {"bar", 1},      //
                    {"baz", 1}});
}

TEST_F(StringPopulationTest, GapsFilled)
{
  create_population(
      {{"A", 2}, {"b", 0}, {"C", 3}, {"d", 0}, {"E", 1}, {"f", 0}, {"G", 1}});
  expect_resampled(
      {{"A", 1}, {"A", 1}, {"C", 1}, {"C", 1}, {"C", 1}, {"E", 1}, {"G", 1}});
}

TEST_F(StringPopulationTest, GapsFilled2)
{
  create_population(
      {{"A", 0}, {"b", 2}, {"C", 0}, {"d", 3}, {"E", 0}, {"f", 2}, {"G", 0}});
  expect_resampled(
      {{"b", 1}, {"b", 1}, {"d", 1}, {"d", 1}, {"d", 1}, {"f", 1}, {"f", 1}});
}

TEST_F(StringPopulationTest, AllFromBegin)
{
  create_population(
      {{"A", 7}, {"b", 0}, {"c", 0}, {"d", 0}, {"e", 0}, {"f", 0}, {"f", 0}});
  expect_resampled(
      {{"A", 1}, {"A", 1}, {"A", 1}, {"A", 1}, {"A", 1}, {"A", 1}, {"A", 1}});
}

TEST_F(StringPopulationTest, AllFromMiddle)
{
  create_population(
      {{"a", 0}, {"b", 0}, {"c", 0}, {"D", 7}, {"e", 0}, {"f", 0}, {"f", 0}});
  expect_resampled(
      {{"D", 1}, {"D", 1}, {"D", 1}, {"D", 1}, {"D", 1}, {"D", 1}, {"D", 1}});
}

TEST_F(StringPopulationTest, AllFromEnd)
{
  create_population(
      {{"a", 0}, {"b", 0}, {"c", 0}, {"d", 0}, {"e", 0}, {"f", 0}, {"F", 7}});
  expect_resampled(
      {{"F", 1}, {"F", 1}, {"F", 1}, {"F", 1}, {"F", 1}, {"F", 1}, {"F", 1}});
}

TEST_F(StringPopulationTest, Shrinking)
{
  create_population(
      {{"A", 2}, {"b", 0}, {"C", 1}, {"d", 0}, {"E", 1}, {"f", 0}, {"G", 1}});
  expect_resampled({{"A", 1}, {"A", 1}, {"C", 1}, {"E", 1}, {"G", 1}});
}

TEST_F(StringPopulationTest, Shrinking2)
{
  create_population(
      {{"A", 0}, {"b", 1}, {"C", 0}, {"d", 3}, {"E", 0}, {"f", 2}, {"G", 0}});
  expect_resampled(
      {{"b", 1}, {"d", 1}, {"d", 1}, {"d", 1}, {"f", 1}, {"f", 1}});
}

TEST_F(StringPopulationTest, Shrinking3)
{
  create_population(
      {{"A", 0}, {"b", 1}, {"C", 0}, {"d", 1}, {"E", 0}, {"f", 1}, {"G", 0}});
  expect_resampled(  //
      {{"b", 1}, {"d", 1}, {"f", 1}});
}

TEST_F(StringPopulationTest, Growing)
{
  create_population(
      {{"A", 2}, {"B", 1}, {"c", 0}, {"D", 2}, {"E", 3}, {"f", 0}, {"G", 1}});
  expect_resampled({{"A", 1},
                    {"A", 1},
                    {"B", 1},
                    {"D", 1},
                    {"D", 1},
                    {"E", 1},
                    {"E", 1},
                    {"E", 1},
                    {"G", 1}});
}

TEST_F(StringPopulationTest, Growing2)
{
  create_population(
      {{"A", 1}, {"b", 0}, {"C", 1}, {"d", 0}, {"E", 1}, {"f", 0}, {"G", 5}});
  expect_resampled({{"A", 1},
                    {"C", 1},
                    {"E", 1},
                    {"G", 1},
                    {"G", 1},
                    {"G", 1},
                    {"G", 1},
                    {"G", 1}});
}

class TestState : public State
{
 public:
  TestState(std::vector<int> s) : spins(s) {}
  void configure(const utils::Json& json) override
  {
    utils::set_from_json(spins, json);
  }
  Structure render() const override { return spins; }
  std::vector<int> spins;
};

TEST(Population, OfStates)
{
  Population<TestState> population;
  population.insert(TestState({1, 2}));
  population.insert(TestState({2, 0}));
  population.insert(TestState({3, 1}));
  for (size_t i = 0; i < population.size(); i++)
  {
    population[i].set_count(population[i]->spins[1]);
  }
  population.resample();
  EXPECT_EQ(population.size(), 3);
  EXPECT_EQ(population[0]->spins[0], 1);
  EXPECT_EQ(population[1]->spins[0], 1);
  EXPECT_EQ(population[2]->spins[0], 3);
}

TEST(Population, OfMetropolis)
{
  //
  Population<Metropolis<TestModel>> metropolis_population;
}
