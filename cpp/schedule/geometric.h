// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <cmath>

#include "schedule/ranged_generator.h"

namespace schedule
{
////////////////////////////////////////////////////////////////////////////////
/// Geometric Schedule
///
/// The schedule (or schedule segment) interpolates geometrically from `initial`
/// to `final` value over the input interval.
///
/// Configuration:
///
/// * Basic definition:
///
///   ```json
///   "schedule": {
///     "type": "geometric",
///     "initial": 1.0,
///     "final": 0.3
///   }
///   ```
///
/// * With input range:
///
///   ```json
///   "schedule": {
///     "type": "geometric",
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
///     "type": "geometric",
///     "initial": 1.0,
///     "final": 0.3
///     "count": 11
///   }
///   ```
///
class Geometric : public RangedGenerator
{
 public:
  Geometric(){};
  Geometric(double initial_value, double final_value);
  /// Get the value at a relative position in the input interval.
  ///
  /// Arguments:
  ///   `progress` -- relative input position \in [0..1]
  ///
  /// Returns:
  ///   the interpolated value
  ///
  double get_progress_value(double progress) const override;

  /// Configure the Geometric generator from input.
  ///
  /// This expects the `initial` and `final` value to interpolate.
  void configure(const utils::Json& json) override;

  /// Return the config value for schedule
  utils::Structure render() const override;
};

}  // namespace schedule
