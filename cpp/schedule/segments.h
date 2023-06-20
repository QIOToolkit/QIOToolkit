// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include "schedule/schedule.h"
#include "schedule/schedule_generator.h"

namespace schedule
{
////////////////////////////////////////////////////////////////////////////////
/// Segments Schedule
///
/// Concatenate multiple schedules to a single schedule.
///
/// Configuration:
///
/// * Basic definition:
///
///   ```json
///   "schedule": {
///     "type": "segments",
///     "segments": [
///       {
///         "type": "constant",
///         "value": 3,
///         "count": 100,
///       },
///       {
///         "type": "linear",
///         "initial": 3,
///         "final": 0.3,
///         "count": 1000
///       },
///       {
///         "type": "constant",
///         "value": 0.3,
///         "count": 500
///       }
///     ]
///   }
///   ```
///
/// * From an array:
///
///   ```json
///   "schedule": {
///     "type": "segments",
///     "segments": [1,2,3]
///   }
///   ```
///
/// * From an array, shorthand
///
///   ```json
///   "schedule": [1,2,3]
///   ```
///
/// NOTE: Unless otherwise specified, each segment is assumed to have
/// a width of 1, with the start point coinciding with the stop of the
/// previous segment. You can modify this default behavior by specifying
/// the (absolute) `stop` point of the segment or by adjusting the width
/// via `count`.
///
/// NOTE: Segments specified as an array of scalar values are presumed
/// to be points of zero input range width spaced by 1. As a result, the
/// total width of the array is mapped to 0..(size-1) and discretization
/// with default count yields back the array. Input values between these
/// points are interpolated linearly.
class Segments : public ScheduleGenerator
{
 public:
  Segments() = default;
  Segments(const std::vector<double>& values);
  /// Get the value at a relative position in the input interval.
  ///
  /// Arguments:
  ///   `progress` -- relative input position \in [0..1]
  ///
  /// Returns:
  ///   the value of the segment covering `progress`.
  ///
  double get_progress_value(double progress) const override;

  /// Configure the Segments generator from input.
  ///
  /// This expects the `segments` to be concatenated.
  void configure(const utils::Json& json) override;

  void adjust();

  /// Return the config value for schedule
  utils::Structure render() const override;

 private:
  std::vector<Schedule> segments_;
};

}  // namespace schedule
