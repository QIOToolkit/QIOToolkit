
#pragma once
#include "utils/component.h"
namespace utils
{
class Range : public Component
{
 public:
  Range();
  Range(double in_initial, double in_final);
  ~Range();

  void configure(const utils::Json& json) override;
  utils::Structure render() const override;
  std::string get_class_name() const override;

  double initial_;
  double final_;
};
}  // namespace utils