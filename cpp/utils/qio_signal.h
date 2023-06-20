// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <queue>

namespace utils
{
class Signal
{
 public:
  enum Type
  {
    UNKNOWN = 0,
    HALT = 1,
    STATUS = 2,
  };

  Signal(Type type);
  static std::queue<Signal>& queue();

  Type get_type();
  double get_time();

 private:
  Type type_;
  double time_;
  static std::queue<Signal> queue_;
};

}  // namespace utils
