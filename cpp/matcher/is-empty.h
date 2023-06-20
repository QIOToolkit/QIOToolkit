// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>

namespace matcher
{
class IsEmptyMatcher
{
 public:
  template <class T>
  bool matches(const T& value) const
  {
    return value.empty();
  }

  template <class T>
  std::string explain(const T& value) const
  {
    if (!matches(value))
    {
      std::stringstream explanation;
      explanation << "must be empty, actual size: " << value.size();
      return explanation.str();
    }
    return "is empty";
  }
};

inline IsEmptyMatcher IsEmpty() { return IsEmptyMatcher(); }

}  // namespace matcher
