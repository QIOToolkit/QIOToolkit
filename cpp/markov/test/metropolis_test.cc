
#include "markov/metropolis.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/random_generator.h"
#include "gtest/gtest.h"
#include "solver/test/test_model.h"

using ::markov::Metropolis;

class MetropolisTest : public ::testing::Test
{
 public:
  MetropolisTest()
  {
    rng.seed(42);
    metropolis.set_model(&model);
    metropolis.set_rng(&rng);
    metropolis.init();
  }

  TestModel model;
  Metropolis<TestModel> metropolis;
  utils::Twister rng;
};

TEST_F(MetropolisTest, SimulatesToyModel)
{
  metropolis.set_temperature(0.1);
  metropolis.init();
  for (int i = 0; i < 50; i++)
  {
    metropolis.make_sweep();
  }
  EXPECT_LT(std::abs(metropolis.cost()), 1);
}
