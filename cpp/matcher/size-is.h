
#pragma once

#include <string>

namespace matcher
{
template <class SizeMatcher>
class SizeIsMatcher
{
 public:
  SizeIsMatcher(const SizeMatcher& size_matcher) : size_matcher_(size_matcher)
  {
  }

  template <class T>
  bool matches(const T& container) const
  {
    return size_matcher_.matches((int)container.size());
  }

  template <class T>
  std::string explain(const T& container) const
  {
    return "size " + size_matcher_.explain((int)container.size());
  }

 private:
  SizeMatcher size_matcher_;
};

template <class Order>
SizeIsMatcher<Order> SizeIs(const Order& order)
{
  return SizeIsMatcher<Order>(order);
}

}  // namespace matcher
