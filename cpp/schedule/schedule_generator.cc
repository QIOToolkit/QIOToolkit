
#include "schedule/schedule_generator.h"

#include "utils/config.h"
#include "matcher/matchers.h"

namespace schedule
{
double ScheduleGenerator::get_initial_value() const
{
  return get_progress_value(0.0);
}

double ScheduleGenerator::get_final_value() const
{
  return get_progress_value(1.0);
}

double ScheduleGenerator::get_start() const
{
  return has_start() ? *start_ : 0;
}

double ScheduleGenerator::get_stop() const { return has_stop() ? *stop_ : 1; }

int ScheduleGenerator::get_count() const
{
  return has_count() ? *count_ : (int)round(std::abs(get_stop() - get_start()));
}

bool ScheduleGenerator::has_start() const { return start_.has_value(); }

bool ScheduleGenerator::has_stop() const { return stop_.has_value(); }

bool ScheduleGenerator::has_count() const { return count_.has_value(); }

bool ScheduleGenerator::is_repeated() const { return is_repeated_; }

void ScheduleGenerator::configure(const utils::Json& json)
{
  this->param(json, "start", start_)
      .description("Minimum expected config value for the generator.");
  this->param(json, "count", count_)
      .description("Number of elements when the schedule is discretized.");
  this->param(json, "stop", stop_)
      .description("Maximum expected config value for the generator.");
  this->param(json, "repeat", is_repeated_)
      .description("Whether to repeated the schedule indefinitely.")
      .default_value(false);
}

}  // namespace schedule
