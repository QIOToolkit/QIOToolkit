// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

namespace matcher
{
template <class M>
class NotMatcher
{
 public:
  NotMatcher(const M& m) : m_(m) {}

  template <class T>
  bool matches(const T& value) const
  {
    return !m_.matches(value);
  }

  template <class T>
  std::string explain(const T& value) const
  {
    return "not " + m_.explain(value);
  }

 private:
  M m_;
};

template <class M>
NotMatcher<M> Not(const M& m)
{
  return NotMatcher<M>(m);
}

}  // namespace matcher
