

#include "utils/range.h"

#include "utils/component.h"
#include "utils/exception.h"
namespace utils
{
Range::~Range() {}
Range::Range() : Range(0., 1.) {}
Range::Range(double in_initial, double in_final)
    : initial_(in_initial), final_(in_final)
{
}

void Range::configure(const utils::Json& json)
{
  this->param(json, "initial", initial_)
      .description("initial value of the range")
      .required();
  this->param(json, "final", final_)
      .description("final value of the range")
      .required();

  if (initial_ > final_)
  {
    THROW(utils::ValueException, "initial value: ", initial_,
          ", is larger than final value:", final_);
  }
}

Structure Range::render() const
{
  Structure s(Structure::OBJECT);
  s["initial"] = initial_;
  s["final"] = final_;
  return s;
}

std::string Range::get_class_name() const { return "Range"; }
}  // namespace utils