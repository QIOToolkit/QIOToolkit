
#include "schedule/constant.h"

#include "utils/config.h"
#include "matcher/matchers.h"

namespace schedule
{
void Constant::configure(const utils::Json& json)
{
  if (json.IsObject())
  {
    ScheduleGenerator::configure(json);
    std::string type;
    this->param(json, "type", type).matches(::matcher::EqualTo("constant"));
    this->param(json, "value", value_).description("constant_value").required();
  }
  else
  {
    utils::set_from_json(value_, json);
  }
}

utils::Structure Constant::render() const
{
  utils::Structure config;
  config["type"] = std::string("constant");
  config["value"] = value_;
  return config;
}

}  // namespace schedule
