// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

#include "utils/component.h"
#include "schedule/schedule_generator.h"

namespace schedule
{
////////////////////////////////////////////////////////////////////////////////
/// Schedule
///
/// A schedule maps an input interval to values for parametrization. This can be
/// employed to, e.g.,
///
///   * specify a temperature set or schedule (tempering, annealing)
///   * define how a parameter should evolve over time (substochastic MC)
///
/// The schedule can be specified in different forms:
///
///   * `constant` value
///   * `linear` or `geometric` interpolation for a range `initial`..`final`
///   * By explicitly listing the temperatures as an array
///
/// Examples:
///
/// * Geometric interpolation of the range `inital` to `final`:
///
///   ```json
///   "schedule": {
///     "type": "geometric",
///     "initial": 1.0
///     "final": 0.3,
///   }
///   ```
///
/// * 20 steps with equal spacing from `initial` to `final`:
///
///   ```json
///   "schedule": {
///     "type": "linear",
///     "initial": 1.0,
///     "final": 3.0
///     "count": 20
///   }
///   ```
///
/// * An explicit set of values:
///
///   ```json
///   "schedule": [0.1, 0.2, 0.3, 0.4]
///   ```
///
/// NOTE: Without a prior call to configure, an object of this class is
/// uninitialized and will throw an access to nullptr exception.
class Schedule : public utils::Component
{
 public:
  Schedule() {}
  Schedule(const Schedule& copy)
  {  // for vector
    if (copy.generator_)
    {
      LOG(FATAL,
          "copy of non-trivial schedule made. This should never happen.");
    }
    assert(!copy.generator_);
  }
  virtual ~Schedule() {}

  Schedule& operator=(const Schedule&) = delete;

  /// Assuming the simulation runs from [0 .. `max_steps`] (inclusive),
  /// returns the temperature to be used at `step`.
  double get_value(double input) const;

  /// Return a set of values for equidistributed inputs over the schedule.
  std::vector<double> get_discretized_values(int count = -1) const;

  /// Get the starting input value
  double get_start() const;

  /// Get the stopping input value
  double get_stop() const;

  /// Get the initial value (at start)
  double get_initial() const { return get_value(get_start()); }

  /// Get the final value (at stop)
  double get_final() const { return get_value(get_stop()); }

  /// Query how many equidistributed inputs this schedule would "naturally"
  /// have (for an explicit set of values, this is the length of the array,
  /// for a generator it is derived from the (rounded) input range.
  int get_count() const;

  /// Force a specific number of steps to be queried from the schedule
  void set_stop(int stop);

  /// Serialization will instantiate the correct `schedule_generator_`
  /// according to / the configuration it is passed.
  void configure(const utils::Json& json) override;

  /// Manually create an linear schedule.
  void make_linear(double initial_value, double final_value);

  /// Manually create an geometric schedule.
  void make_geometric(double beta_start, double beta_stop);

  // Build up list of parameters from segs
  void make_list(const std::vector<double>& values);

  // Set up a constant schedule from value
  void make_constant(double value);

  utils::Structure render() const override;

 private:
  friend class Segments;
  std::unique_ptr<ScheduleGenerator> generator_;
};

}  // namespace schedule
