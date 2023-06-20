// Copyright (c) Microsoft. All Rights Reserved.
#include "schedule/geometric.h"

#include "utils/config.h"
#include "utils/exception.h"

namespace schedule
{
double Geometric::get_progress_value(double progress) const
{
  return initial_ * pow(final_ / initial_, progress);
}

void Geometric::configure(const utils::Json& json)
{
  std::string type;
  this->param(json, "type", type)
      .required()
      .matches(::matcher::EqualTo("geometric"));
  RangedGenerator::configure(json);
  if (initial_ <= 0)
  {
    throw utils::ValueException("initial must be positive for geometric");
  }
  if (final_ <= 0)
  {
    throw utils::ValueException("final must be positive for geometric");
  }
}

Geometric::Geometric(double initial_value, double final_value)
{
  initial_ = initial_value;
  final_ = final_value;
  Geometric();
}

/// Return the config value for schedule
utils::Structure Geometric::render() const
{
  utils::Structure config;
  config["type"] = std::string("geometric");
  config["initial"] = initial_;
  config["final"] = final_;
  return config;
}

}  // namespace schedule
