// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/config.h"

#include "utils/exception.h"
#include "gtest/gtest.h"

TEST(Config, FeatureSwitch)
{
  utils::initialize_features();
  EXPECT_EQ(false,
            utils::feature_disabled(utils::Features::FEATURE_COMPACT_TERMS));
  EXPECT_EQ(true, utils::feature_enabled(utils::Features::FEATURE_COMPACT_TERMS));
  EXPECT_EQ(true,
            utils::feature_enabled(utils::Features::FEATURE_ACCURATE_TIMEOUT));
  std::set<int> disabled_feature = {utils::Features::FEATURE_COMPACT_TERMS,
                                    utils::Features::FEATURE_ACCURATE_TIMEOUT};
  utils::set_disabled_feature(disabled_feature);
  EXPECT_EQ(true,
            utils::feature_disabled(utils::Features::FEATURE_COMPACT_TERMS));
  EXPECT_EQ(false,
            utils::feature_enabled(utils::Features::FEATURE_COMPACT_TERMS));
  EXPECT_EQ(true,
            utils::feature_disabled(utils::Features::FEATURE_ACCURATE_TIMEOUT));
  EXPECT_EQ(false,
            utils::feature_enabled(utils::Features::FEATURE_ACCURATE_TIMEOUT));
}

TEST(Config, WrongFeature)
{
  utils::initialize_features();
  std::set<int> wrong_feature = {-1};
  EXPECT_THROW(utils::set_disabled_feature(wrong_feature),
               utils::ConfigurationException);
  std::set<int> wrong_feature2 = {utils::Features::FEATURE_COUNT + 10};
  EXPECT_THROW(utils::set_disabled_feature(wrong_feature2),
               utils::ConfigurationException);
}

TEST(Config, UseMemSavingFalse)
{
  utils::initialize_features();
  EXPECT_TRUE(
      utils::feature_disabled(utils::Features::FEATURE_USE_MEMORY_SAVING));
  EXPECT_FALSE(
      utils::feature_enabled(utils::Features::FEATURE_USE_MEMORY_SAVING));
}