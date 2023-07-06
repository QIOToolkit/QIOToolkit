
#include "utils/json.h"

#include <cfloat>
#include <cmath>
#include <string>
#include <vector>

#include "utils/component.h"
#include "utils/config.h"
#include "utils/exception.h"
#include "gtest/gtest.h"

using utils::ConfigurationException;
using utils::Error;
using utils::Json;
using utils::Structure;

class TestComponent : public utils::Component
{
 public:
  std::string name;
  int id;
  void configure(const utils::Json& json) override
  {
    this->param(json, "name", name).description("name of the component");
    this->param(json, "id", id).description("id of the component");
  }
};

class JsonTest : public testing::Test
{
 protected:
  TestComponent component;
};

TEST_F(JsonTest, ReadsScalars)
{
  auto json = utils::json_from_string(R"(
    {
      "a": true,
      "b": false,
      "c": 0,
      "d": 42,
      "e": -13,
      "f": 3.5,
      "g": -0.3,
      "h": -0,
      "i": "foo",
      "j": "",
      "k": null
    }
  )");

  bool a, b;
  int c, d, e;
  double f, g, h, k;
  std::string i, j;
  component.param(json, "a", a).description("parameter a");
  EXPECT_EQ(a, true);
  component.param(json, "b", b).description("parameter b");
  EXPECT_EQ(b, false);
  component.param(json, "c", c).description("parameter c");
  EXPECT_EQ(c, 0);
  component.param(json, "d", d).description("parameter d");
  EXPECT_EQ(d, 42);
  component.param(json, "e", e).description("parameter e");
  EXPECT_EQ(e, -13);
  component.param(json, "f", f).description("parameter f");
  EXPECT_EQ(f, 3.5);
  component.param(json, "g", g).description("parameter g");
  EXPECT_EQ(g, -0.3);
  component.param(json, "h", h).description("parameter h");
  EXPECT_EQ(h, -0.0);
  component.param(json, "i", i).description("parameter i");
  EXPECT_EQ(i, "foo");
  component.param(json, "j", j).description("parameter j");
  EXPECT_EQ(j, "");
  component.param(json, "k", k).description("parameter k");
  std::stringstream s, snan;
  s << k;
  snan << NAN;
  EXPECT_EQ(s.str(), snan.str());
}

TEST_F(JsonTest, ReadsVectors)
{
  std::vector<int> v = {0, 0};
  auto json = utils::json_from_string("[1,2,3]");
  utils::set_from_json(v, json);
  EXPECT_EQ(v.size(), 3);
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 2);
  EXPECT_EQ(v[2], 3);
}

TEST_F(JsonTest, ReadsNonVector)
{
  std::vector<int> v = {0, 0};
  auto json = utils::json_from_string(R"({"a": 42, "b": -13, "c": 0})");
  EXPECT_THROW_MESSAGE(utils::set_from_json(v, json), utils::ValueException,
                       "Expected array");
}

TEST_F(JsonTest, ReadsMaps)
{
  std::map<std::string, int> mi = {{"nope", 0}};
  auto json = utils::json_from_string(R"({"a": 42, "b": -13, "c": 0})");
  utils::set_from_json(mi, json);
  EXPECT_EQ(mi.size(), 3);
  EXPECT_EQ(mi["a"], 42);
  EXPECT_EQ(mi["b"], -13);
  EXPECT_EQ(mi["c"], 0);
}

TEST_F(JsonTest, ReadsComponent)
{
  auto json = utils::json_from_string(R"({"name": "foo", "id": 42})");
  TestComponent test;
  utils::set_from_json(test, json);
  EXPECT_EQ(test.name, "foo");
  EXPECT_EQ(test.id, 42);
}

TEST_F(JsonTest, TypeMismatch)
{
  auto json = utils::json_from_string(R"(
    {
      "boolean": true,
      "integer": 42,
      "dbl": 3.5,
      "str": "foo",
      "array": [],
      "object": {}
    }
  )");
  bool boolean;
  int integer;

  EXPECT_THROW_MESSAGE(
      component.param(json, "boolean", integer)
          .description("parameter integer"),
      ConfigurationException,
      "parameter `boolean`: expected int32, found true (boolean)");
  EXPECT_THROW_MESSAGE(
      component.param(json, "integer", boolean)
          .description("parameter boolean"),
      ConfigurationException,
      "parameter `integer`: expected boolean, found 42 (int32)");
  EXPECT_THROW_MESSAGE(
      component.param(json, "dbl", boolean).description("parameter boolean"),
      ConfigurationException,
      "parameter `dbl`: expected boolean, found 3.5 (double)");
  EXPECT_THROW_MESSAGE(
      component.param(json, "str", boolean).description("parameter boolean"),
      ConfigurationException,
      "parameter `str`: expected boolean, found 'foo' (string)");
  EXPECT_THROW_MESSAGE(
      component.param(json, "array", boolean).description("parameter boolean"),
      ConfigurationException,
      "parameter `array`: expected boolean, found array");
  EXPECT_THROW_MESSAGE(
      component.param(json, "object", boolean).description("parameter boolean"),
      ConfigurationException,
      "parameter `object`: expected boolean, found object");
  EXPECT_THROW_MESSAGE(  //
      component.param(json, "dbl", integer).description("a whole number"),
      ConfigurationException,
      "parameter `dbl`: expected int32, found 3.5 (double)");
}
