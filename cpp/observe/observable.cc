// Copyright (c) Microsoft. All Rights Reserved.
#include "observe/observable.h"

#include <cmath>

#include "utils/exception.h"

namespace observe
{
using ::utils::Structure;
using ::utils::ValueException;

void Observable::record(double value) { this->record(value, 1.0); }

void Observable::record(int value) { this->record((double)value, 1.0); }

void Observable::record(size_t value) { this->record((double)value, 1.0); }

void Constant::reset() { value_ = NAN; }

void Constant::record(double value, double weight)
{
  if (weight > 0)
  {
    value_ = value;
  }
}

Structure Constant::render() const { return value_; }

void Average::reset()
{
  weight_ = 0;
  sum_ = 0;
}

void Average::record(double value, double weight)
{
  weight_ += weight;
  sum_ += weight * value;
}

Structure Average::render() const { return weight_ > 0 ? sum_ / weight_ : NAN; }

Structure AverageAndCount::render() const
{
  utils::Structure s;
  s["mean"] = Average::render();
  s["count"] = weight_;
  return s;
}

void MinMax::reset()
{
  Average::reset();
  min_ = std::numeric_limits<double>::infinity();
  max_ = -min_;
}

void MinMax::record(double value, double weight)
{
  Average::record(value, weight);
  if (weight < 0)
  {
    if (value == min_) min_ = std::numeric_limits<double>::infinity();
    if (value == max_) max_ = -std::numeric_limits<double>::infinity();
  }
  else
  {
    // NOTE: We are deliberately handling weight == 0 as well here; it
    // is used by Windowed<MinMax> to signal a change in extreme value.
    if (value < min_) min_ = value;
    if (value > max_) max_ = value;
  }
}

Structure MinMax::render() const
{
  utils::Structure s = AverageAndCount::render();
  if (weight_ > 0)
  {
    s["min"] = min_;
    s["max"] = max_;
  }
  else
  {
    s["min"] = NAN;
    s["max"] = NAN;
  }
  return s;
}

void Average2::reset()
{
  MinMax::reset();
  sum2_ = 0;
}

void Average2::record(double value, double weight)
{
  MinMax::record(value, weight);
  sum2_ += weight * value * value;
}

Structure Average2::render() const
{
  auto s = MinMax::render();
  if (weight_ != 0)
  {
    s["dev"] = sqrt((sum2_ - sum_ * sum_ / weight_) / weight_);
  }
  else
  {
    s["dev"] = NAN;
  }
  return s;
}

void RangedDistribution::reset()
{
  Average2::reset();
  for (auto& bin : bins_) bin = 0;
}

void RangedDistribution::record(double value, double weight)
{
  Average2::record(value, weight);
  if (value >= low_ && value < high_)
  {
    size_t bin = static_cast<size_t>(floor(static_cast<double>(bins_.size()) *
                                           (value - low_) / (high_ - low_)));
    bins_[bin] += weight;
  }
}

Structure RangedDistribution::render() const
{
  auto s = Average2::render();
  for (size_t i = 0; i < bins_.size(); i++)
  {
    Structure bin;
    bin["value"] = low_ + (static_cast<double>(i) + 0.5) * (high_ - low_) /
                              static_cast<double>(bins_.size());
    bin["count"] = bins_[i];
    s["bins"].push_back(bin);
  }
  return s;
}

void BinWidthDistribution::reset()
{
  Average2::reset();
  bins_.clear();
}

void BinWidthDistribution::record(double value, double weight)
{
  Average2::record(value, weight);
  int bin = (int)round(value / bin_width_);
  if (bins_.find(bin) == bins_.end())
  {
    bins_[bin] = weight;
  }
  else
  {
    bins_[bin] += weight;
  }
  if (bins_[bin] == 0)
  {
    bins_.erase(bin);
  }
}

Structure BinWidthDistribution::render() const
{
  auto s = Average2::render();
  for (const auto& value_count : bins_)
  {
    Structure bin;
    bin["value"] = value_count.first * bin_width_;
    bin["count"] = value_count.second;
    s["bins"].push_back(bin);
  }
  return s;
}

void Windowed::reset()
{
  weight_ = 0;
  queue_.clear();
  sorted_.clear();
  observable_->reset();
}

void Windowed::record(double value, double weight)
{
  if (weight <= 0)
  {
    std::stringstream err;
    err << get_class_name() << " cannot handle negative weight in: record("
        << value << ", " << weight << ")";
    throw ValueException(err.str());
  }
  observable_->record(value, weight);
  weight_ += weight;
  queue_.emplace_back(value, weight);
  sorted_.insert(value);
  while (weight_ > window_size_ && !queue_.empty())
  {
    const auto& remove = queue_.front();
    observable_->record(remove.value, -remove.weight);
    sorted_.erase(sorted_.find(remove.value));
    weight_ -= remove.weight;
    if (!sorted_.empty() && remove.value < *sorted_.begin())
    {
      observable_->record(*sorted_.begin(), 0);
    }
    if (!sorted_.empty() && remove.value > *sorted_.rbegin())
    {
      observable_->record(*sorted_.rbegin(), 0);
    }
    queue_.pop_front();
  }
}

Structure Windowed::render() const { return observable_->render(); }

void Batched::reset()
{
  weight_ = 0;
  batches_.clear();
  observable_->reset();
}

void Batched::record(double value, double weight)
{
  observable_->record(value, weight);
  weight_ += weight;
  if (weight_ >= batch_size_)
  {
    auto rendered = observable_->render();
    batches_.push_back(rendered);
    observable_->reset();
    weight_ = 0;
  }
}

Structure Batched::render() const
{
  return batches_;
}

void LogBatched::reset()
{
  weight_ = 0;
  batches_.clear();
  observable_->reset();
}

void LogBatched::record(double value, double weight)
{
  observable_->record(value, weight);
  weight_ += weight;
  if (weight_ >= current_batch_size_)
  {
    auto rendered = observable_->render();
    batches_.push_back(rendered);
    observable_->reset();
    weight_ = 0;
    current_batch_size_ *= 2;
  }
}

Structure LogBatched::render() const
{
  return batches_;
}

void Milestone::reset()
{
  key_total_ = 0;
  keys_.clear();
  values_.clear();
}

void Milestone::record(double value, double partial_key)
{
  key_total_ += partial_key;
  if (values_.empty() || value != values_.back())
  {
    keys_.push_back(key_total_);
    values_.push_back(value);
  }
}

Structure Milestone::render() const
{
  Structure s(Structure::ARRAY);
  for (size_t i = 0; i < values_.size(); i++)
  {
    s[i][key_label_] = keys_[i];
    s[i][value_label_] = values_[i];
  }
  return s;
}

}  // namespace observe
