// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

namespace matcher
{
template <class A, class B>
class AllOfMatcher
{
 public:
  AllOfMatcher(const A& a, const B& b) : a_(a), b_(b) {}

  template <class T>
  bool matches(const T& value) const
  {
    return a_.matches(value) && b_.matches(value);
  }

  template <class T>
  std::string explain(const T& value) const
  {
    if (!a_.matches(value))
    {
      return a_.explain(value);
    }
    else if (!b_.matches(value))
    {
      return b_.explain(value);
    }
    return a_.explain(value) + " and " + b_.explain(value);
  }

 private:
  A a_;
  B b_;
};

template <class A, class B>
AllOfMatcher<A, B> AllOf(const A& a, const B& b)
{
  return AllOfMatcher<A, B>(a, b);
}

}  // namespace matcher
