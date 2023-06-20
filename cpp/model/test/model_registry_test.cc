// Copyright (c) Microsoft. All Rights Reserved.
#include "gtest/gtest.h"
#include "model/all_models.h"

TEST(ModelRegistry, InstantiatesClockModel)
{
  EXPECT_TRUE(model::ModelRegistry::has("clock"));
  auto* my_model = model::ModelRegistry::create("clock");
  EXPECT_EQ(my_model->get_identifier(), "clock");
}

TEST(ModelRegistry, InstantiatesPottsModel)
{
  EXPECT_TRUE(model::ModelRegistry::has("potts"));
  auto* my_model = model::ModelRegistry::create("potts");
  EXPECT_EQ(my_model->get_identifier(), "potts");
}

TEST(ModelRegistry, ThrowsOnUnknownModel)
{
  EXPECT_FALSE(model::ModelRegistry::has("unknown"));
  EXPECT_THROW(model::ModelRegistry::create("unknown"),
               utils::KeyDoesNotExistException);
}
