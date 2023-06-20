// Copyright (c) Microsoft. All Rights Reserved.
#include "examples/descent.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "utils/random_generator.h"
#include "examples/soft_spin.h"
#include "gtest/gtest.h"

using ::utils::ConfigurationException;
using ::utils::Json;

using ::examples::Descent;
using ::examples::SoftSpin;

class SoftDescentTest : public testing::Test
{
 public:
  void configure(const std::string& params_data)
  {
    auto input = utils::json_from_string(R"({
      "cost_function": {
        "type": "softspin",
        "version": "0.1",
        "terms": [
          {"c": 1, "ids": [0, 1]},
          {"c": 1, "ids": [1, 2]},
          {"c": 1, "ids": [2, 0]}
        ]
      }
    })");
    model_.configure(input);
    model_.init();
    auto params = utils::json_from_string(params_data);
    solver_.set_model(&model_);
    solver_.configure(params);
    solver_.init();
  }

 protected:
  SoftSpin model_;
  Descent<SoftSpin> solver_;
};

TEST_F(SoftDescentTest, ApproachesGroundState)
{
  configure(R"({
    "type": "descent",
    "version": "0.1",
    "params": {
      "seed": 5,
      "step_limit": 50,
      "samples": 20
    }
  })");
  solver_.run();
  double ground_state = -1.0;
  EXPECT_LT(solver_.get_lowest_cost(), ground_state + 0.01);
}
