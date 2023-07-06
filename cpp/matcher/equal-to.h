
#pragma once

#include <sstream>
#include <string>

#include "utils/repr.h"

namespace matcher
{
template <class T>
class EqualToMatcher
{
 public:
  EqualToMatcher(const T& expected) : expected_(expected) {}

  template <class V>
  bool matches(const V& value) const
  {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic warning "-Wsign-compare"
    return value == expected_;
    #pragma GCC diagnostic pop
  }

  template <class V>
  std::string explain(const V& value) const
  {
    std::stringstream explanation;
    if (matches(value))
    {
      explanation << "is equal to " << repr(expected_);
    }
    else
    {
      explanation << "must be equal to " << repr(expected_) << ", found "
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
EqualToMatcher<T> EqualTo(T value)
{
  return EqualToMatcher<T>(value);
}

}  // namespace matcher
