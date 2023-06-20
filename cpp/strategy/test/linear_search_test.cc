// Copyright (c) Microsoft. All Rights Reserved.
#include <cmath>
#include <sstream>
#include <string>
#include "gtest/gtest.h"
#include "strategy/linear_search.h"


using strategy::LinearSearchOpt;

class Problem
{
 public:
  Problem(double target_val) : root(target_val) {}
  double objective(int x) { return (x - root) * (x - root); }
  double root;
};

TEST(LinearSearchTest, FindMinimum)
{
  LinearSearchOpt opt;
  opt.init(1, 333);
  opt.set_ranges({10, 20, 30, 50});
  double target_val = 34;
  double best_param = 0;
  Problem pr(target_val);
  int nsteps = 20;
  double min_obj = std::numeric_limits<double>::max();
  std::vector<double> param_vec(1);
  for (int i = 0; i < nsteps; i++)
  {
    opt.recommend_parameter_values(param_vec);
    double objective = pr.objective(param_vec[0]);
    opt.add_new_sample(param_vec, objective);
    
    if (objective < min_obj)
    {
      best_param = std::lround(param_vec[0]);
      min_obj = objective;
    }
  }
  EXPECT_NEAR(best_param, target_val, 5);
  EXPECT_NEAR(min_obj, 0.0, 25);
}
