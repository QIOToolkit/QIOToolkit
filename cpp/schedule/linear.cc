
#include "schedule/linear.h"

#include "utils/config.h"

namespace schedule
{
Linear::Linear(double initial_value, double final_value)
{
  initial_ = initial_value;
  final_ = final_value;
  Linear();
}

double Linear::get_progress_value(double progress) const
{
  return initial_ + progress * (final_ - initial_);
}

void Linear::configure(const utils::Json& json)
{
  std::string type;
  this->param(json, "type", type)
      .required()
      .matches(::matcher::EqualTo("linear"));
  RangedGenerator::configure(json);
}

utils::Structure Linear::render() const
{
  utils::Structure config;
  config["type"] = std::string("linear");
  config["initial"] = initial_;
  config["final"] = final_;
  return config;
}
}  // namespace schedule
