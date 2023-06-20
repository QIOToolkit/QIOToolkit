// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include "utils/component.h"
#include "utils/json.h"
#include "utils/structure.h"

namespace strategy
{
const char* const kNumMultistarts = "num_multistarts";
const char* const kMaxNumSteps = "max_num_steps";
const char* const kMaxNumRestarts = "max_num_restarts";
const char* const kGamma = "gamma";
const char* const kPreMult = "pre_mult";
const char* const kMaxRelativeChange = "max_relative_change";
const char* const kTolerance = "tolerance";
const char* const kNumStepsAveraged = "num_steps_averaged";
const char* const kMaxMCSteps = "max_mc_steps";
const char* const kNumToSample = "num_to_sample";
const char* const kInitialSamples = "initial_samples";
const char* const kNonImprovementLimit = "nonimprovement_limit";
const char* const kReservedSamples = "reserved_samples";
class GDParameters : public utils::Component
{
 public:
  int num_multistarts_;
  int max_num_steps_;
  int max_num_restarts_;
  int num_steps_averaged_;
  int max_mc_steps_;
  double gamma_;
  double pre_mult_;
  double max_relative_change_;
  double tolerance_;
  int num_to_sample_;

  void load(const utils::Structure& params);
  void configure(const utils::Json& params) override;
  std::string to_string() const;
};
}  // namespace strategy
