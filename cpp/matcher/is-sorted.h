
#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "utils/repr.h"

namespace matcher
{
class Ascending
{
 public:
  template <class A, class B>
  bool in_order(const A& a, const B& b)
  {
    return a <= b;
  }
  std::string explain() { return "ascending"; }
  std::string reverse() { return ">"; }
};

class StrictlyAscending
{
 public:
  template <class A, class B>
  bool in_order(const A& a, const B& b)
  {
    return a < b;
  }
  std::string explain() { return "strictly ascending"; }
  std::string reverse() { return ">="; }
};

class Descending
{
 public:
  template <class A, class B>
  bool in_order(const A& a, const B& b)
  {
    return a >= b;
  }
  std::string explain() { return "descending"; }
  std::string reverse() { return "<"; }
};

class StrictlyDescending
{
 public:
  template <class A, class B>
  bool in_order(const A& a, const B& b)
  {
    return a > b;
  }
  std::string explain() { return "strictly descending"; }
  std::string reverse() { return "<="; }
};

template <class Order>
class IsSortedMatcher
{
 public:
  IsSortedMatcher(const Order& order) : order_(order) {}

  template <class T>
  bool matches(const std::vector<T>& v) const
  {
    for (int i = 1; i < v.size(); i++)
    {
      if (!order_.in_order(v[i - 1], v[i]))
      {
        return false;
      }
    }
  }

  template <class T>
  std::string explain(const std::vector<T>& v) const
  {
    for (int i = 1; i < v.size(); i++)
    {
      if (!order_.in_order(v[i - 1], v[i]))
      {
        std::stringstream explanation;
        explanation << "must be sorted " << order_.explain() << ", ";
        explanation << "found " << v[i - 1] << order_.reverse() << v[i];
        explanation << " at positions " << (i - 1) << ", " << i << ".";
        return explanation.str();
      }
    }
    return "is " + order_.explain();
  }

 private:
  Order order_;
};

template <class Order>
IsSortedMatcher<Order> IsSorted(const Order& order)
{
  return IsSortedMatcher<Order>(order);
}

}  // namespace matcher
