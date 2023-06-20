// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>

#include "utils/repr.h"

namespace matcher
{
template <class T>
class GreaterEqualMatcher
{
 public:
  GreaterEqualMatcher(const T& expected) : expected_(expected) {}

  template <class V>
  bool matches(const V& value) const
  {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic warning "-Wsign-compare"
    return value >= expected_;
    #pragma GCC diagnostic pop
  }

  template <class V>
  std::string explain(const V& value) const
  {
    std::stringstream explanation;
    if (matches(value))
    {
      explanation << "is greater-equal " << repr(expected_);
    }
    else
    {
      explanation << "must be greater-equal " << repr(expected_) << ", found "
                  << repr(value);
    }
    return explanation.str();
  }

 private:
  T expected_;
};

////////////////////////////////////////////////////////////////////////////////
/// Infer the type of predicate from the value being passed.
template <class T>
GreaterEqualMatcher<T> GreaterEqual(T value)
{
  return GreaterEqualMatcher<T>(value);
}

}  // namespace matcher
