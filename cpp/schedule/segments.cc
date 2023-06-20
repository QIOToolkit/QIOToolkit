// Copyright (c) Microsoft. All Rights Reserved.
#include "schedule/segments.h"

#include <algorithm>

#include "utils/config.h"
#include "utils/exception.h"
#include "schedule/constant.h"
#include "schedule/schedule.h"

namespace schedule
{
using utils::ValueException;

double Segments::get_progress_value(double progress) const
{
  double position = get_start() + progress * (get_stop() - get_start());
  if (position < segments_.front().get_start())
  {
    return segments_.front().generator_->get_initial_value();
  }
  for (size_t i = 0; i < segments_.size(); i++)
  {
    double stop = segments_[i].get_stop();
    if (position > stop) continue;
    double start = segments_[i].get_start();
    if (i > 0 && position < start)
    {
      double last_stop = segments_[i - 1].get_stop();
      double last_value = segments_[i - 1].generator_->get_final_value();
      double next_value = segments_[i].generator_->get_initial_value();
      progress = (position - last_stop) / (start - last_stop);
      return (1 - progress) * last_value + progress * next_value;
    }
    else
    {
      return segments_[i].get_value(position);
    }
  }
  return segments_.back().generator_->get_final_value();
}

namespace
{
bool is_constant_point(ScheduleGenerator* generator)
{
  return dynamic_cast<Constant*>(generator) && !generator->has_stop() &&
         !generator->has_count();
}

}  // namespace

Segments::Segments(const std::vector<double>& values)
{
  segments_.resize(values.size());
  for (size_t i = 0; i < values.size(); i++)
  {
    segments_[i].make_constant(values[i]);
  }
  count_ = (int)values.size();
}

void Segments::adjust()
{
  // `last stop` denotes where the last segment ended
  // (or where it ended +1 if it was a constant point).
  double last_stop = segments_[0].generator_->get_start();
  for (size_t i = 0; i < segments_.size(); i++)
  {
    auto* current = segments_[i].generator_.get();

    // If not specified explicitly, assume the start of the new
    // segments is where we stopped last.
    if (!current->has_start()) current->start_ = last_stop;
    // Check that input ranges are not overlapping (i.e., this segment does
    // not start before the last one stops).
    if (i > 0 && current->get_start() < segments_[i - 1].generator_->get_stop())
    {
      throw ValueException("Overlapping segments");
    }

    // Decide where this segment stop and the next one should start.
    if (is_constant_point(current))
    {
      // For a constant point, make its width 0 but add a default gap of
      // with 1 to the next segment.
      current->stop_ = current->get_start();
      last_stop = current->get_stop() + 1;
    }
    else if (!current->has_stop())
    {
      // For non-points, choose width according to count if an explicit
      // (absolute) stop point was not given.
      int count = current->get_count();
      current->stop_ = current->get_start() + count;
      last_stop = current->get_stop();
    }
    // Check that the segment does not stop before it starts.
    if (current->get_stop() < current->get_start())
    {
      throw ValueException("Negative width segment");
    }
  }

  // Adjust the start, stop and count of the concatenated segments.
  if (!has_start())
  {
    // if not specified explicitly, assume input range starts at the
    // start of the first segment.
    start_ = segments_.front().get_start();
  }
  if (!has_stop())
  {
    // if not specified explicitly, assume input range ends at the
    // stop of the last segment.
    stop_ = segments_.back().get_stop();
  }
  if (!has_count())
  {
    // if not specified, assume the count is given by the range of
    // input values.
    //
    // NOTE: `fencepost` is {0,1} to adjust for the fencepost error.
    // (I.e., if the last value was a constant point, we add an offset of +1 to
    // ensure the right number of discretized values is generated).
    int fencepost = (int)floor(last_stop - segments_.back().get_stop());
    count_ = (int)round(get_stop() - get_start() + fencepost);
  }
}

void Segments::configure(const utils::Json& json)
{
  if (json.IsObject())
  {
    std::string type;
    this->param(json, "type", type)
        .required()
        .matches(::matcher::EqualTo("segments"));
    this->param(json, "segments", segments_)
        .description("array describing the segments")
        .required();
  }
  else
  {
    set_from_json(segments_, json);
    // config.handle(segments_);
    if (segments_.size() == 0)
    {
      throw ValueException(
          "`segments` must be specified as a non-empty array.");
    }
  }

  adjust();
}

utils::Structure Segments::render() const
{
  utils::Structure config;
  config["type"] = std::string("segments");
  std::vector<utils::Structure> segs;
  segs.reserve(segments_.size());
  for (size_t i = 0; i < segments_.size(); i++)
  {
    segs.push_back(segments_[i]);
  }

  config["segments"] = segs;
  return config;
}
}  // namespace schedule
