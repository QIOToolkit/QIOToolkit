// Copyright (c) Microsoft. All Rights Reserved.
#include "schedule/ranged_generator.h"

#include "utils/config.h"

namespace schedule
{
void RangedGenerator::configure(const utils::Json& json)
{
  ScheduleGenerator::configure(json);
  this->param(json, "initial", initial_)
      .description("initial value of the ranged generator")
      .required();
  this->param(json, "final", final_)
      .description("final value of the ranged generator")
      .required();
}

}  // namespace schedule
