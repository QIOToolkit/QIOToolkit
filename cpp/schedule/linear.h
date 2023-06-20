// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include "schedule/ranged_generator.h"

namespace schedule
{
////////////////////////////////////////////////////////////////////////////////
/// Linear Schedule
///
/// The schedule (or schedule segment) interpolates linearly from `initial` to
/// `final` value over the input interval.
///
/// Configuration:
///
/// * Basic definition:
///
///   ```json
///   "schedule": {
///     "type": "linear",
///     "initial": 1.0,
///     "final": 0.3
///   }
///   ```
///
/// * With input range:
///
///   ```json
///   "schedule": {
///     "type": "linear",
///     "initial": 1.0,
///     "final": 0.3
///     "start": 0,
///     "stop": 10
///   }
///   ```
///
/// * With count:
///
///   ```json
///   "schedule": {
///     "type": "linear",
///     "initial": 1.0,
///     "final": 0.3
///     "count": 11
///   }
///   ```
///
class Linear : public RangedGenerator
{
 public:
  Linear() {}

  /// Explicitly create an inverse linear schedule
  ///
  /// This allows algorithms to explicitly create an inverse linear schedule
  /// from other input variants (i.e., `beta_start`, `beta_stop`).
  Linear(double initial_value, double final_value);

  /// Get the value at a relative position in the input interval.
  ///
  /// Arguments:
  ///   `progress` -- relative input position \in [0..1]
  ///
  /// Returns:
  ///   the interpolated value
  ///
  double get_progress_value(double progress) const override;

  /// Configure the Linear generator from input.
  ///
  /// This expects the `initial` and `final` value to interpolate.
  void configure(const utils::Json& json) override;

  /// Return the config value for schedule
  utils::Structure render() const override;
};

}  // namespace schedule
