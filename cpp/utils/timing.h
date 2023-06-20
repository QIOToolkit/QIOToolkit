// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <stdint.h>

#include <string>

#include "utils/log.h"

namespace utils
{
/// Return the number of seconds since epoch,
/// with microsecond resolution
double get_wall_time();

// return an unsigned int-casted number of microseconds since epoch.
unsigned int get_seed_time();

/// Return a cpu seconds counter (to compute cpu time);
double get_cpu_time();

/// Drop in timer (logs time used to INFO when the object leaves scope)
class ScopedTimer
{
 public:
  ScopedTimer(std::string title) : title_(title), start_(get_wall_time()) {}
  ~ScopedTimer() { LOG(INFO, title_, ": ", get_wall_time() - start_, "s"); }

 private:
  std::string title_;
  double start_;
};

}  // namespace utils
