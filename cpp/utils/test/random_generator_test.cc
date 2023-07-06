
#include "utils/random_generator.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using utils::ConfigurationException;
using ::utils::PCG;
using ::utils::Twister;
using ::testing::ElementsAre;

TEST(Twister, DefaultSeed)
{
  Twister r;
  EXPECT_EQ(int(r.uniform() * 100), 13);
  EXPECT_EQ(int(r.uniform() * 100), 83);
  EXPECT_EQ(int(r.uniform() * 100), 96);
}

TEST(Twister, CustomSeed)
{
  Twister r(42);
  EXPECT_EQ(int(r.uniform() * 100), 79);
  EXPECT_EQ(int(r.uniform() * 100), 18);
  EXPECT_EQ(int(r.uniform() * 100), 77);
  r.seed(42);
  EXPECT_EQ(int(r.uniform() * 100), 79);
  EXPECT_EQ(int(r.uniform() * 100), 18);
  EXPECT_EQ(int(r.uniform() * 100), 77);
}

TEST(PCG, DefaultSeed)
{
  PCG r;
  EXPECT_EQ(int(r.uniform() * 100), 9);
  EXPECT_EQ(int(r.uniform() * 100), 83);
  EXPECT_EQ(int(r.uniform() * 100), 5);
}

TEST(PCG, CustomSeed)
{
  PCG r(42);
  EXPECT_EQ(int(r.uniform() * 100), 41);
  EXPECT_EQ(int(r.uniform() * 100), 26);
  EXPECT_EQ(int(r.uniform() * 100), 40);
  r.seed(42);
  EXPECT_EQ(int(r.uniform() * 100), 41);
  EXPECT_EQ(int(r.uniform() * 100), 26);
  EXPECT_EQ(int(r.uniform() * 100), 40);
}

TEST(PCG, Fork)
{
  PCG r(42);
  auto f1 = r.fork();
  auto f2 = r.fork();
  auto f3 = r.fork();

  std::vector<int> v0, v1, v2, v3;

  for (int i = 0; i < 12; i++)
  {
    v0.push_back(int(r.uniform() * 100));
    v1.push_back(int(f1->uniform() * 100));
    v2.push_back(int(f2->uniform() * 100));
    v3.push_back(int(f3->uniform() * 100));
  }

  EXPECT_THAT(v0, ElementsAre(95, 79, 48, 70, 2, 89, 70, 10, 27, 66, 47, 18));
  EXPECT_THAT(v1, ElementsAre(86, 95, 8, 65, 36, 92, 17, 14, 82, 9, 90, 19));
  EXPECT_THAT(v2, ElementsAre(79, 27, 7, 13, 39, 85, 26, 66, 89, 5, 55, 13));
  EXPECT_THAT(v3, ElementsAre(14, 82, 1, 6, 90, 27, 67, 82, 94, 24, 76, 35));
}
