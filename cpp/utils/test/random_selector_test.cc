// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/random_selector.h"

#include <algorithm>

#include "gtest/gtest.h"

using utils::RandomSelector;

TEST(RandomSelector, Equiprobable)
{
  for (size_t size = 2; size < 20; size++)
  {
    RandomSelector selector;
    for (size_t i = 0; i < size; i++)
    {
      selector.insert(i, 1);
    }

    for (size_t i = 0; i < size; i++)
    {
      EXPECT_EQ(selector.select(i / double(size) + 1e-7), i);
      EXPECT_EQ(selector.select((i + .999) / double(size)), i);
    }
  }
}

TEST(RandomSelector, AddingTo1)
{
  RandomSelector selector;
  selector.insert(0, .42);
  selector.insert(1, .58);
  EXPECT_EQ(selector.select(0), 0);
  EXPECT_EQ(selector.select(.4199), 0);
  EXPECT_EQ(selector.select(.42), 1);
  EXPECT_EQ(selector.select(.9999), 1);
}

TEST(RandomSelector, AddingTo10)
{
  RandomSelector selector;
  std::vector<double> weights = {1, 2, 3, 4};
  for (int i = 0; i < (int)weights.size(); i++)
  {
    selector.insert(i, weights[i]);
  }

  EXPECT_EQ(selector.select(0), 0);
  EXPECT_EQ(selector.select(0.0999), 0);
  EXPECT_EQ(selector.select(0.1), 1);
  EXPECT_EQ(selector.select(0.2999), 1);
  EXPECT_EQ(selector.select(0.3), 2);
  EXPECT_EQ(selector.select(0.5999), 2);
  EXPECT_EQ(selector.select(0.6), 3);
  EXPECT_EQ(selector.select(0.999), 3);
}

TEST(RandomSelector, Bucketed)
{
  RandomSelector selector;
  std::vector<double> weights = {100, .42, .13, 1 / 3., 17};
  double total_weight = 0;
  for (size_t i = 0; i < weights.size(); i++)
  {
    selector.insert(i, weights[i]);
    total_weight += weights[i];
  }

  // Select and remove the first element.
  EXPECT_EQ(selector.select_and_remove(0), 0);
  total_weight -= weights[0];
  weights[0] = 0;

  // Select a random one of the remaining elements 1000x
  for (size_t j = 0; j < 1000; j++)
  {
    weights[selector.select(static_cast<double>(j) / 1000.)] -=
        total_weight / 1000.;
  }

  // Check that all weights were reduced to zero
  // (within the precision the 1000x allows).
  for (auto weight : weights)
  {
    EXPECT_LT(std::abs(weight), total_weight / 1000.);
  }
}

TEST(RandomSelector, RemoveFirst)
{
  size_t size = 15;
  RandomSelector selector;
  for (size_t i = 0; i < size; i++)
  {
    selector.insert(i, 1 / double(size));
  }

  // this will remove element 0, followed by reverse ordering
  // (from the back) because selecting the first always puts
  // the last element in its place.
  EXPECT_EQ(selector.select_and_remove(0), 0);
  for (size_t i = 1; i < size; i++)
  {
    EXPECT_EQ(selector.select_and_remove(0), size - i);
  }
}

TEST(RandomSelector, SelectLast)
{
  size_t size = 15;
  RandomSelector selector;
  for (size_t i = 0; i < size; i++)
  {
    selector.insert(i, 1 / double(size));
  }

  // this will remove element 0, followed by reverse ordering
  // (from the back) because selecting the first always puts
  // the last element in its place.
  for (size_t i = 0; i < size; i++)
  {
    EXPECT_EQ(selector.select_and_remove(0.999), size - 1 - i);
  }
}

TEST(RandomSelector, RemoveOrDoNotAdd)
{
  size_t size = 10;
  RandomSelector selector0;
  RandomSelector selector1;

  // Removing element or not adding the same element,
  // for the current implementation,
  // should lead to the same results.

  // Create dataset and remove some element
  for (size_t i = 1; i < size; i++)
  {
    selector1.insert(i, double(i));
  }
  size_t id = selector1.select_and_remove(0.9999);

  // Create dataset without same element
  for (size_t i = 1; i < size; i++)
  {
    if (i != id)
      selector0.insert(i, double(i));
    else
      selector0.insert(size - 1, double(size - 1));
  }

  // Result structures should be the same
  EXPECT_EQ(selector1.select_and_remove(0.6), selector0.select_and_remove(0.6));
}

TEST(RandomSelector, UniformDistributionCheck)
{
  // test if RandomSelector follows uniform distribution
  // for instance, check if there is no bias toward last element
  size_t size = 75;
  RandomSelector selector;

  for (size_t i = 0; i < size; i++)
  {
    selector.insert(i, 1.0);
  }

  for (size_t i = 0; i < size; i++)
  {
    double uniform = (double(i) + 0.5) / double(size);
    size_t id = selector.select(uniform);
    EXPECT_EQ(id, i);
  }
}

TEST(RandomSelector, AddRealArray)
{
  // extracted real data from other test where
  // result of select function was out of bound
  // when expected to be closer to the middle
  // which was very suspicious.

  std::vector<double> weights = {
      0.035613099453075003,  0.052431845033121371,  0.97940354700162879,
      0.051567855886273772,  0.022903864963628173,  0.0050188220144968820,
      0.045327497779740433,  0.95989490168673397,   0.035282073490468191,
      0.97473274506301155,   0.036095985013232523,  0.0025030127094891608,
      0.013173879013652856,  0.013754875843456915,  0.056531730315894091,
      0.026699509794074316,  0.026436544979190346,  0.059493908721991895,
      0.025710312850090400,  0.028382321765067609,  0.055460315564139329,
      0.0036758875048159556, 0.059498446940953187,  0.015960129599996264,
      0.041476436452129173,  0.015406582081598952,  0.036588888394347929,
      0.96193046590757425,   0.042058099119007597,  0.91673878881683502,
      0.020338298281435074,  0.90138043180391403,   0.042934074025681390,
      0.0037819651518704589, 0.99988852169993903,   0.98785471715372342,
      0.90384829536743738,   0.037186851228543816,  0.97232442480207437,
      0.98305168528757902,   0.93390006672899539,   0.99393587718866450,
      0.052998247395941167,  0.017021975910986642,  0.99074871513846563,
      0.024606364629831123,  0.029276647444614445,  0.038575024523113344,
      0.99861545466597901,   0.92067067065070296,   0.93726236652148731,
      0.024319952013970969,  0.055167123531712825,  0.042710737610802241,
      0.048048455805725698,  0.019800817511951507,  0.95081231879162642,
      0.029478392528955677,  0.049822407757686316,  0.015742065542287964,
      0.012587983427772231,  0.0042269547398576801, 0.9836614367384631};

  RandomSelector selector;
  // Create dataset and remove some element
  double sum = 0.0;
  std::vector<double> cummulative;
  for (size_t i = 0; i < weights.size(); i++)
  {
    selector.insert(i + 1, weights[i]);
    sum += weights[i];
    cummulative.push_back(sum);
  }

  for (size_t i = 0; i < cummulative.size(); i++)
  {
    cummulative[i] /= sum;
  }

  // last number too small
  for (double uniform = 0.01011; uniform < 1.0; uniform += 0.1)
  {
    int expected_id =
        std::upper_bound(cummulative.begin(), cummulative.end(), uniform) -
        cummulative.begin() + 1;
    int id = selector.select(uniform);
    EXPECT_EQ(id, expected_id);
  }
}

TEST(RandomSelector, SelectFromProgressiveArray)
{
  // Test select function for array with simple to track uneven weights

  std::vector<double> ids = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  std::vector<double> weights = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  RandomSelector selector;

  // Create dataset and remove some element
  double sum = 0.0;
  std::vector<double> cummulative;
  for (size_t i = 0; i < weights.size(); i++)
  {
    selector.insert(ids[i], weights[i]);
    sum += weights[i];
    cummulative.push_back(sum);
  }

  for (size_t i = 0; i < cummulative.size(); i++)
  {
    cummulative[i] /= sum;
  }

  for (double uniform = 0.01011; uniform < 1.0; uniform += 0.1)
  {
    int expected_id =
        std::upper_bound(cummulative.begin(), cummulative.end(), uniform) -
        cummulative.begin() + 1;
    int id = selector.select(uniform);
    EXPECT_EQ(id, expected_id);
  }
}

TEST(RandomSelector, RemoveAll)
{
  // Test for checking how RandomSelector behaving around empty state.

  std::vector<double> ids = {1, 2, 3, 4};
  std::vector<double> weights = {1, 2, 3, 4};

  RandomSelector selector;

  for (size_t i = 0; i < weights.size(); i++)
    selector.insert(ids[i], weights[i]);

  EXPECT_EQ(selector.select_and_remove(0.5), 3);
  EXPECT_EQ(selector.select_and_remove(0.5), 4);
  EXPECT_EQ(selector.select_and_remove(0.5), 2);
  EXPECT_EQ(selector.select_and_remove(0.5), 1);

  selector.insert(2, 2);
  EXPECT_EQ(selector.select_and_remove(0.1), 2);

  for (size_t i = 0; i < weights.size(); i++)
    selector.insert(ids[i], weights[i]);

  EXPECT_EQ(selector.select_and_remove(0.5), 3);
  EXPECT_EQ(selector.select_and_remove(0.5), 4);
  EXPECT_EQ(selector.select_and_remove(0.5), 2);
  EXPECT_EQ(selector.select_and_remove(0.5), 1);
}
