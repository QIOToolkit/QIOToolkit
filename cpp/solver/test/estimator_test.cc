
#include "solver/estimator.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "utils/random_generator.h"
#include "gtest/gtest.h"
#include "markov/model.h"
#include "model/ising.h"
#include "solver/test/test_model.h"

using ::utils::Json;
using ::model::Ising;
using ::solver::Estimator;

class EstimatorTest : public testing::Test
{
 public:
  EstimatorTest()
  {
    std::string head = R"({
      "cost_function": {
        "type": "ising",
        "version": "1.0",
        "terms": [)";

    std::string edges = R"(
          {"c":  0.128789, "ids": [0, 1]}, 
          {"c":  1.873590, "ids": [0, 2]}, 
          {"c": -1.909049, "ids": [0, 3]}, 
          {"c": -1.221736, "ids": [0, 5]}, 
          {"c":  0.364197, "ids": [0, 9]}, 
          {"c": -0.128786, "ids": [1, 4]}, 
          {"c":  0.144797, "ids": [1, 6]}, 
          {"c": -0.436185, "ids": [1, 9]}, 
          {"c": -0.420332, "ids": [2, 3]}, 
          {"c": -0.039110, "ids": [2, 4]}, 
          {"c":  0.598417, "ids": [2, 6]}, 
          {"c":  0.268915, "ids": [2, 8]}, 
          {"c":  1.191451, "ids": [3, 5]}, 
          {"c":  0.962035, "ids": [3, 6]}, 
          {"c": -1.943090, "ids": [3, 7]}, 
          {"c":  0.379814, "ids": [3, 8]}, 
          {"c":  1.643106, "ids": [4, 6]}, 
          {"c":  0.512239, "ids": [4, 8]}, 
          {"c": -0.032975, "ids": [5, 9]}, 
          {"c":  0.637845, "ids": [6, 9]}, 
          {"c":  0.945639, "ids": [7, 8]}, 
          {"c": -0.350328, "ids": [7, 9]})";

    // strong_edges = 10x edges
    std::string strong_edges = R"(
          {"c":  1.28789, "ids": [0, 1]}, 
          {"c":  18.73590, "ids": [0, 2]}, 
          {"c": -19.09049, "ids": [0, 3]}, 
          {"c": -12.21736, "ids": [0, 5]}, 
          {"c":  3.64197, "ids": [0, 9]}, 
          {"c": -1.28786, "ids": [1, 4]}, 
          {"c":  1.44797, "ids": [1, 6]}, 
          {"c": -4.36185, "ids": [1, 9]}, 
          {"c": -4.20332, "ids": [2, 3]}, 
          {"c": -0.39110, "ids": [2, 4]}, 
          {"c":  5.98417, "ids": [2, 6]}, 
          {"c":  2.68915, "ids": [2, 8]}, 
          {"c":  11.91451, "ids": [3, 5]}, 
          {"c":  9.62035, "ids": [3, 6]}, 
          {"c": -19.43090, "ids": [3, 7]}, 
          {"c":  3.79814, "ids": [3, 8]}, 
          {"c":  16.43106, "ids": [4, 6]}, 
          {"c":  5.12239, "ids": [4, 8]}, 
          {"c": -0.32975, "ids": [5, 9]}, 
          {"c":  6.37845, "ids": [6, 9]}, 
          {"c":  9.45639, "ids": [7, 8]}, 
          {"c": -3.50328, "ids": [7, 9]})";

    std::string extended = edges + R"(,
          {"c":  0.128789, "ids": [10, 11]}, 
          {"c":  1.873590, "ids": [10, 12]}, 
          {"c": -1.909049, "ids": [10, 13]}, 
          {"c": -1.221736, "ids": [10, 15]}, 
          {"c":  0.364197, "ids": [10, 19]}, 
          {"c": -0.128786, "ids": [11, 14]}, 
          {"c":  0.144797, "ids": [11, 16]}, 
          {"c": -0.436185, "ids": [11, 19]}, 
          {"c": -0.420332, "ids": [12, 13]}, 
          {"c": -0.039110, "ids": [12, 14]}, 
          {"c":  0.598417, "ids": [12, 16]}, 
          {"c":  0.268915, "ids": [12, 18]}, 
          {"c":  1.191451, "ids": [13, 15]}, 
          {"c":  0.962035, "ids": [13, 16]}, 
          {"c": -1.943090, "ids": [13, 17]}, 
          {"c":  0.379814, "ids": [13, 18]}, 
          {"c":  1.643106, "ids": [14, 16]}, 
          {"c":  0.512239, "ids": [14, 18]}, 
          {"c": -0.032975, "ids": [15, 19]}, 
          {"c":  0.637845, "ids": [16, 19]}, 
          {"c":  0.945639, "ids": [17, 18]}, 
          {"c": -0.350328, "ids": [17, 19]})";

    std::string shift = R"(,
          {"c": 20, "ids": []})";

    std::string foot = R"(
        ]
      }
    })";

    ising.configure(utils::json_from_string(head + edges + foot));
    scaled_ising.configure(utils::json_from_string(head + strong_edges + foot));
    shifted_ising.configure(
        utils::json_from_string(head + edges + shift + foot));
    extended_ising.configure(utils::json_from_string(head + extended + foot));
  }

  double relative_error(double a, double b)
  {
    return std::abs(b - a) / std::abs(b);
  }

  double relative_deviation(std::vector<double> values)
  {
    double sum1 = 0, sum2 = 0;
    for (auto value : values)
    {
      sum1 += value;
      sum2 += value * value;
    }
    sum1 /= static_cast<double>(values.size());
    sum2 /= static_cast<double>(values.size());
    return sqrt(sum2 - sum1 * sum1) / std::abs(sum1);
  }

  Ising ising;
  Ising scaled_ising;
  Ising shifted_ising;
  Ising extended_ising;
  utils::Twister rng;
};

TEST_F(EstimatorTest, AnalyzeIsing)
{
  // This analyzes the basic ising model defined by the edges above.
  Estimator<Ising> estimator;
  rng.seed(42);
  auto result = estimator.analyze(ising, rng);
  EXPECT_EQ(result.to_string(),
            R"({"count": 10, "final": 0.301291, "initial": 1.858353})");
}

TEST_F(EstimatorTest, RngSensitivity)
{
  Estimator<Ising> estimator;
  std::vector<double> T_initial, T_final, count;
  for (uint32_t seed = 0; seed < 32; seed++)
  {
    rng.seed(seed);
    auto sample = estimator.analyze(ising, rng);
    T_initial.push_back(sample["initial"].get<double>());
    T_final.push_back(sample["final"].get<double>());
    count.push_back(static_cast<double>(sample["count"].get<int>()));
  }
  EXPECT_LT(relative_deviation(T_initial), 0.13);
  EXPECT_LT(relative_deviation(T_final), 0.30);
  EXPECT_LT(relative_deviation(count), 0.15);
}

TEST_F(EstimatorTest, AnalyzeStrong)
{
  // This model has all edge costs multiplied by 10. The effect should
  // be an equal stretching of the energy scale (and thus intial and
  // final temperature).
  Estimator<Ising> estimator;
  rng.seed(42);
  auto base = estimator.analyze(ising, rng);
  rng.seed(42);
  auto scaled = estimator.analyze(scaled_ising, rng);
  EXPECT_FLOAT_EQ(scaled["initial"].get<double>(),
                  10 * base["initial"].get<double>());
  EXPECT_FLOAT_EQ(scaled["final"].get<double>(),
                  10 * base["final"].get<double>());
  EXPECT_EQ(scaled["count"].get<int>(), base["count"].get<int>());
}

TEST_F(EstimatorTest, AnalyzeShifted)
{
  Estimator<Ising> estimator;
  rng.seed(42);
  auto base = estimator.analyze(ising, rng);
  rng.seed(42);
  auto shifted = estimator.analyze(shifted_ising, rng);
  EXPECT_FLOAT_EQ(shifted["initial"].get<double>(),
                  base["initial"].get<double>());
  EXPECT_FLOAT_EQ(shifted["final"].get<double>(), base["final"].get<double>());
  EXPECT_EQ(shifted["count"].get<int>(), base["count"].get<int>());
}

TEST_F(EstimatorTest, AnalyzeExtended)
{
  // This checks how the behavior of the estimator changes with different
  // system sizes. The extended system is the same as the base with two
  // equivalent disconnected graphs.
  Estimator<Ising> estimator;
  rng.seed(42);
  auto base = estimator.analyze(ising, rng);
  rng.seed(42);
  auto extended = estimator.analyze(extended_ising, rng);
  // We expect the characteristic temperatures to remain comparable
  EXPECT_LE(relative_error(extended["initial"].get<double>(),
                           base["initial"].get<double>()),
            0.20);
  EXPECT_LE(relative_error(extended["final"].get<double>(),
                           base["final"].get<double>()),
            0.30);
  // For a system with 2x the variables we expect the count to be twice
  // the size (the cost range to cover is roughly twice as large while the
  // std dev for a single transition cost remains roughly the same).
  EXPECT_LE(relative_error(static_cast<double>(extended["count"].get<int>()),
                           2 * static_cast<double>(base["count"].get<int>())),
            0.30);
}
