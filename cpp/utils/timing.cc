// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/timing.h"

#include <time.h>

#include <chrono>

#include "utils/operating_system.h"

namespace utils
{
using namespace std::chrono;

double get_wall_time()
{
  return (double(duration_cast<microseconds>(
                     high_resolution_clock::now().time_since_epoch())
                     .count()) /
          double(1000000));
}

unsigned int get_seed_time()
{
  return (unsigned int)duration_cast<microseconds>(
             high_resolution_clock::now().time_since_epoch())
      .count();
}

double get_cpu_time() { return (double)clock() / CLOCKS_PER_SEC; }

}  // namespace utils
