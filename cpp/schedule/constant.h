
#pragma once

#include "schedule/schedule_generator.h"

namespace schedule
{
////////////////////////////////////////////////////////////////////////////////
/// Constant Schedule
///
/// The schedule (or schedule segment) has a constant value over the input
/// range. If queried for values outside the input range, the same constant
/// value is returned as well.
///
/// Configuration Examples:
///
/// * Basic definition:
///   ```json
///   "schedule": {
///     "type": "constant",
///     "value": 42.0
///   }
///   ```
///
/// * Short-hand notation:
///   ```json
///   "schedule": 42.0
///   ```
///
/// * With input range:
///   ```json
///   "schedule": {
///     "type": "constant",
///     "value": 42.0,
///     "start": 0,
///     "stop": 13
///   }
///   ```
///
/// * With count:
///   ```json
///   "schedule": {
///     "type": "constant",
///     "value": 42.0,
///     "count": 100
///   }
///   ```
///
/// NOTE: Since the value of this generator is constant, the properties
/// `start`, `stop` and `count` only affect how many times the value is
/// repeated when used as a `Segment` or generating a discretized set.
///
class Constant : public ScheduleGenerator
{
 public:
  Constant(double value) : ScheduleGenerator(), value_(value) {}
  Constant(const Constant& other) : ScheduleGenerator(), value_(other.value_) {}
  Constant() = default;
  /// Get the value at a relative position in the input interval.
  ///
  /// Arguments:
  ///   `progress` -- relative input position \in [0..1]
  ///
  /// Returns:
  ///   the (constant) value of the generator.
  ///
  double get_progress_value(double) const override { return value_; }

  /// Configure the Constant generator from input.
  ///
  /// This expects either the long (object) or short (scalar) input format,
  /// as described in the class description.
  void configure(const utils::Json& json) override;

  /// Return the config value for schedule
  utils::Structure render() const override;

 private:
  double value_;
};

}  // namespace schedule
