// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

namespace matcher
{
class AnyMatcher
{
 public:
  template <class T>
  bool matches(const T&) const
  {
    return true;
  }

  template <class T>
  std::string explain(const T& value) const
  {
    return "can be any value, found " + repr(value);
  }
};

inline AnyMatcher Any() { return AnyMatcher(); }

}  // namespace matcher
