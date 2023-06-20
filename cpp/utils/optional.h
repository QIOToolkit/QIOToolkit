// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#if __cplusplus < 201703L

#include <iostream>

namespace std
{
template <class T>
class optional
{
 public:
  optional() : has_value_(false) {}
  bool has_value() const { return has_value_; }
  const T& operator=(const T& value)
  {
    has_value_ = true;
    return value_ = value;
  }

  const T& value() const { return value_; }
  const T& operator*() const { return value_; }
  const T& operator->() const { return value_; }

  void reset() { has_value_ = false; }

 private:
  T value_;
  bool has_value_;
};

}  // namespace std

#endif

namespace std
{
template <class T>
ostream& operator<<(ostream& out, const optional<T>& t)
{
  if (t.has_value())
  {
    return out << t;
  }
  else
  {
    return out << "unset";
  }
}

}  // namespace std
