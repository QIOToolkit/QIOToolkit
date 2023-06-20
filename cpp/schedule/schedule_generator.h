// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <cmath>

#include "utils/component.h"
#include "utils/optional.h"
#include "utils/structure.h"

namespace schedule
{
/// ScheduleGenerator interface
///
/// A schedule generator maps any input value in the progress interval [0..1]
/// to an output value.
///
/// NOTE: The input is always in the range [0..1], because the mapping
/// from the input interval to this progress interval is handled by the
/// `Schedule` class. This ensures the input mapping logic is not duplicated
/// in every generator.

class ScheduleGenerator : public utils::Component
{
 public:
  ScheduleGenerator() : is_repeated_(false) {}
  virtual ~ScheduleGenerator() {}

  /// Return the value that `progress` is mapped to.
  virtual double get_progress_value(double progress) const = 0;

  /// Return the value at the beginning of the input interval.
  double get_initial_value() const;

  /// Return the value at the end of the input interval.
  double get_final_value() const;

  /// Get the `start` input value (first in the input interval).
  /// [This default to start=0 if not specified].
  double get_start() const;

  /// Get the `stop` input value (last in the input interval).
  /// [This defaults to stop=1 if not specified].
  double get_stop() const;

  /// Get the number of equidistributed input values to use when generating
  /// a discretized set from this generator (without requesting a specific
  /// set size).
  /// The default value of count=1 means pick just one (the initial value)
  int get_count() const;

  /// Force the number of equidistributed values to pick
  void set_stop(int stop) { stop_ = stop; }

  /// Query if the start value was explicitly configured.
  bool has_start() const;

  /// Query if the stop value was explicitly configured.
  bool has_stop() const;

  /// Query if the count value was explicitly configured.
  bool has_count() const;

  /// Should this schedule be repeated indefinitly outside the input interval?
  bool is_repeated() const;

  void configure(const utils::Json& json) override;

 protected:
  friend class Segments;
  std::optional<double> start_;
  std::optional<double> stop_;
  std::optional<int> count_;
  bool is_repeated_;
};

}  // namespace schedule
