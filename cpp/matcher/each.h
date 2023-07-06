
#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace matcher
{
template <class ElementMatcher>
class EachMatcher
{
 public:
  EachMatcher(ElementMatcher element_matcher)
      : element_matcher_(element_matcher)
  {
  }

  template <class T>
  bool matches(const std::vector<T>& container) const
  {
    for (const auto& element : container)
    {
      if (!element_matcher_.matches(element))
      {
        return false;
      }
    }
    return true;
  }

  template <class T>
  std::string explain(const std::vector<T>& container) const
  {
    if (container.size() == 0)
    {
      return "is empty (always true)";
    }
    for (size_t i = 0; i < container.size(); i++)
    {
      if (!element_matcher_.matches(container[i]))
      {
        std::stringstream explanation;
        explanation << "each element " + element_matcher_.explain(container[i]);
        explanation << " at position " << i;
        return explanation.str();
      }
    }
    return "each element " + element_matcher_.explain(container[0]);
  }

 private:
  ElementMatcher element_matcher_;
};

// Infer the type from the predicate.
template <class ElementMatcher>
EachMatcher<ElementMatcher> Each(ElementMatcher element_matcher)
{
  return EachMatcher<ElementMatcher>(element_matcher);
}

}  // namespace matcher
