

#include "utils/optional.h"

#include "utils/component.h"
#include "utils/config.h"
#include "utils/json.h"
#include "gtest/gtest.h"

using utils::Json;

TEST(Optional, Integer)
{
  std::optional<int> number;
  EXPECT_FALSE(number.has_value());

  number = 5;
  EXPECT_EQ(number.value(), 5);
  EXPECT_TRUE(number.has_value());

  number.reset();
  EXPECT_FALSE(number.has_value());
}

TEST(Optional, Json)
{
  auto null = utils::json_from_string("null");
  std::optional<int> number;
  number = 42;
  utils::set_from_json(number, null);
  EXPECT_FALSE(number.has_value());

  auto thirteen = utils::json_from_string("13");
  utils::set_from_json(number, thirteen);
  EXPECT_TRUE(number.has_value());
  EXPECT_EQ(number.value(), 13);
}

TEST(Optional, Parameter)
{
  auto in = utils::json_from_string(R"(
{
  "foo": 42,
  "bar": null
})");

  std::optional<int> foo, bar, baz;
  foo.reset();
  bar = 1;
  baz = 2;

  utils::Component component;
  component.param(in, "foo", foo).description("foo should have a value");
  component.param(in, "bar", bar)
      .description("bar should have no value (null)");
  component.param(in, "baz", baz)
      .description("baz should have no value (unset)");

  EXPECT_TRUE(foo.has_value());
  EXPECT_EQ(foo.value(), 42);
  EXPECT_FALSE(bar.has_value());
  EXPECT_FALSE(baz.has_value());
}
