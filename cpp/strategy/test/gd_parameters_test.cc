
#include "strategy/gd_parameters.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "utils/structure.h"
#include "gtest/gtest.h"

using strategy::GDParameters;
TEST(GDParametersTest, Load)
{
  utils::Structure s;
  s["gamma"] = 0.5;
  s["pre_mult"] = 1.0e-3;
  s["max_relative_change"] = 1.0;
  s["tolerance"] = 1.0e-6;
  s["num_multistarts"] = 100;
  s["max_num_steps"] = 200;
  s["max_num_restarts"] = 20;
  s["num_steps_averaged"] = 20;
  s["max_mc_steps"] = 2;
  s["num_to_sample"] = 3;

  GDParameters param;
  param.load(s);
  EXPECT_DOUBLE_EQ(s["gamma"].get<double>(), param.gamma_);
  EXPECT_DOUBLE_EQ(s["pre_mult"].get<double>(), param.pre_mult_);
  EXPECT_DOUBLE_EQ(s["max_relative_change"].get<double>(),
                   param.max_relative_change_);
  EXPECT_DOUBLE_EQ(s["tolerance"].get<double>(), param.tolerance_);
  EXPECT_EQ(s["num_multistarts"].get<int>(), param.num_multistarts_);
  EXPECT_EQ(s["max_num_steps"].get<int>(), param.max_num_steps_);
  EXPECT_EQ(s["max_num_restarts"].get<int>(), param.max_num_restarts_);
  EXPECT_EQ(s["num_steps_averaged"].get<int>(), param.num_steps_averaged_);
  EXPECT_EQ(s["max_mc_steps"].get<int>(), param.max_mc_steps_);
  EXPECT_EQ(s["num_to_sample"].get<int>(), param.num_to_sample_);
}