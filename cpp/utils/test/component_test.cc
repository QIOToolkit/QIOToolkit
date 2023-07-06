
#include "utils/component.h"

#include "utils/exception.h"
#include "gtest/gtest.h"

TEST(Component, NamesSelf)
{
  utils::Component component;
  EXPECT_EQ(component.get_class_name(), "utils::Component");
  std::stringstream out;
  EXPECT_THROW_MESSAGE(out << component, utils::RuntimeException,
                       "utils::Component::render() is not implemented");
}

class Derived : public utils::Component
{
};

TEST(Component, NamesDerived)
{
  Derived derived;
  EXPECT_EQ(derived.get_class_name(), "Derived");
}

template <class T>
class Templated : public utils::Component
{
};

TEST(Component, NamesTemplated)
{
  Templated<int> templated;
  EXPECT_EQ(templated.get_class_name(), "Templated<int>");
}

TEST(Component, NamesTemplatedByClass)
{
  Templated<Derived> templated;
  EXPECT_EQ(templated.get_class_name(), "Templated<Derived>");
}

TEST(Component, NamesNestedTemplates)
{
  Templated<Templated<int>> nested;
  EXPECT_EQ(nested.get_class_name(), "Templated<Templated<int>>");
}

TEST(Component, SizeOf)
{
  // This test is to alert developers of unintendedly increasing the size of
  // utils::Component, which is the base of A LOT of objects in qiotoolkit
  // (notably, graph::Edge and graph::Node).
  //
  // IMPORTANT: If you need to change the number in the following line, make
  // sure this does not have a severe performance or memory footprint impact.
  static_assert(sizeof(utils::Component) == 8,
                "Change of size of utils::Component, which is the base of A LOT "
                "of objects.");
}
