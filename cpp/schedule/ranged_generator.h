
#pragma once

#include "schedule/schedule_generator.h"

namespace schedule
{
/// RangedGenerator base class
///
/// This is a shared interface for generators returning values
/// in a configured range `initial`..`final`.
class RangedGenerator : public ScheduleGenerator
{
 public:
  RangedGenerator() : initial_(0), final_(0) {}

  /// Shared input format: Two numbers defining the range `initial`..`final`.
  void configure(const utils::Json& json) override;

 protected:
  double initial_;
  double final_;
};

}  // namespace schedule
