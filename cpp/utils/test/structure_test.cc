// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/component.h"
#include "utils/structure.h"

#include <cmath>

#include "gtest/gtest.h"

using utils::Structure;

TEST(Structure, Unset)
{
  Structure s;
  EXPECT_EQ(s.to_string(), "?");
}

TEST(Structure, Boolean)
{
  Structure s(true);
  EXPECT_EQ(s.to_string(), "true");
  s.clear();
  s = true;
  EXPECT_EQ(s.to_string(), "true");
}

TEST(Structure, integer)
{
  Structure s(42);
  EXPECT_EQ(s.to_string(), "42");
  s = -13;
  EXPECT_EQ(s.to_string(), "-13");
}

TEST(Structure, int64)
{
  Structure s((uint64_t)2147483648);
  EXPECT_EQ(s.to_string(), "2147483648");
  s = (uint64_t)4294967296;
  EXPECT_EQ(s.to_string(), "4294967296");
}

TEST(Structure, double)
{
  Structure s(3.14159);
  EXPECT_EQ(s.to_string(), "3.141590");
  s = NAN;
  std::stringstream ss;
  ss << "nan";
  EXPECT_EQ(s.to_string(), ss.str());
}

TEST(Structure, string)
{
  Structure hello("world");
  EXPECT_EQ(hello.to_string(), "\"world\"");
}

TEST(Structure, array)
{
  Structure s = std::vector<int>();
  EXPECT_EQ(s.to_string(), "[]");
  s = std::vector<int>({1, 2, 3});
  EXPECT_EQ(s.to_string(), R"([1,2,3])");
}

class Foo : public utils::Component {
  public:
    utils::Structure render() const override { return "foo"; }
};

TEST(Structure, ArrayOfObjects) {
  std::vector<Foo> foo_vector = {Foo(), Foo(), Foo()};
  utils::Structure foo_structure = foo_vector;
  EXPECT_EQ(foo_structure.to_string(), R"(["foo","foo","foo"])");
  utils::Structure nested;
  nested["foos"] = foo_vector;
  EXPECT_EQ(nested.to_string(), R"({"foos": ["foo","foo","foo"]})");
}

TEST(Structure, object)
{
  Structure s = std::map<std::string, int>();
  EXPECT_EQ(s.to_string(), "{}");
  s = std::map<std::string, int>({{"foo", 1}, {"bar", 2}, {"baz", 3}});
  EXPECT_EQ(s.to_string(), R"({"bar": 2, "baz": 3, "foo": 1})");
}

TEST(Structure, quoting)
{
  Structure s;
  s = std::map<std::string, std::string>({{"With\\Slash", "With\"Quote"}});
  EXPECT_EQ(s.to_string(), "{\"With\\\\Slash\": \"With\\\"Quote\"}");
}

TEST(Structure, subcategory_creation)
{
  Structure s;
  // create category
  s["a"] = "first";
  // create another category and subcategory
  s["b"]["c"] = "second";
  EXPECT_EQ(s.to_string(), R"({
  "a": "first",
  "b": {"c": "second"}
})");
}

TEST(Structure, sorts_numeric)
{
  // sorts numerically if all numbers
  Structure s;
  s["2"] = "first";
  s["10"] = "second";
  EXPECT_EQ(s.to_string(), R"({"2": "first", "10": "second"})");

  // doesn't sort if not all numbers
  s["a"] = "non-number";
  EXPECT_EQ(s.to_string(),
            R"({"10": "second", "2": "first", "a": "non-number"})");
}

TEST(Structure, deep_copy_string)
{
  std::unique_ptr<Structure> old(new Structure());
  auto content = std::string("test to ensure deep copy work fine!");
  *old = content;
  Structure new1(*old);
  Structure new2;
  new2.push_back(1);
  new2.push_back(2);
  new2 = *old;
  old.release();
  EXPECT_EQ("\"test to ensure deep copy work fine!\"", new1.to_string(false));
  EXPECT_EQ("\"test to ensure deep copy work fine!\"", new2.to_string(false));
}

TEST(Structure, deep_copy_array)
{
  std::unique_ptr<Structure> old(new Structure());
  for (int i = 0; i < 100; i++)
  {
    (*old)[i] = i;
  }
  Structure new1(*old);
  Structure new2;
  new2["content"] = 2.0;
  new2 = *old;

  old.release();
  EXPECT_EQ(80, new1[80].get<int>());
  EXPECT_EQ(88, new2[88].get<int>());
}

TEST(Structure, deep_copy_object)
{
  std::unique_ptr<Structure> old(new Structure());
  Structure level2;
  level2["level2"] = (int)2;
  Structure level1;
  level1["level1"] = level2;
  (*old)["level0"] = level1;
  Structure new1(*old);
  Structure new2 = std::string("new2");
  new2 = *old;
  old.release();
  EXPECT_EQ(2, new1["level0"]["level1"]["level2"].get<int>());
  EXPECT_EQ(2, new2["level0"]["level1"]["level2"].get<int>());
}

TEST(Structure, deep_copy_basics)
{
  std::unique_ptr<Structure> old_bool(new Structure());
  *old_bool = false;
  Structure new1(*old_bool);
  Structure new2;
  new2["bool"] = true;
  new2 = *old_bool;
  old_bool.release();
  EXPECT_EQ(false, new1.get<bool>());
  EXPECT_EQ(false, new2.get<bool>());

  std::unique_ptr<Structure> old_int(new Structure());
  *old_int = -1;
  Structure new3(*old_int);
  Structure new4;
  new4 = false;
  new4 = *old_int;
  old_int.release();
  EXPECT_EQ(-1, new3.get<int>());
  EXPECT_EQ(-1, new4.get<int>());

  std::unique_ptr<Structure> old_int64(new Structure());
  *old_int64 = (int64_t)(-2);
  Structure new5(*old_int64);
  Structure new6;
  new6 = std::string("new 6 string");
  new6 = *old_int64;
  old_int64.release();
  EXPECT_EQ(-2, new5.get<int64_t>());
  EXPECT_EQ(-2, new6.get<int64_t>());
}

TEST(Structure, is_empty)
{
  Structure s(Structure::OBJECT);
  EXPECT_EQ(true, s.is_empty());
  s["empty"] = false;
  EXPECT_EQ(false, s.is_empty());
  EXPECT_EQ(true, s.has_key("empty"));
  EXPECT_EQ(false, s.has_key("none"));
  s.remove("empty");
  EXPECT_EQ(true, s.is_empty());

  Structure a(Structure::ARRAY);
  EXPECT_EQ(true, a.is_empty());
  EXPECT_EQ(0, a.get_array_size());
  a.push_back(0);
  EXPECT_EQ(false, a.is_empty());
}

TEST(Structure, null_output)
{
  Structure s1(Structure::OBJECT);
  EXPECT_EQ("{}", s1.to_string(true));
  EXPECT_EQ("{}", s1.to_string(false));
  Structure s2(Structure::ARRAY);
  EXPECT_EQ("[]", s2.to_string(true));
  EXPECT_EQ("[]", s2.to_string(false));
  Structure s3(Structure::STRING);
  EXPECT_EQ("\"\"", s3.to_string(true));
  EXPECT_EQ("\"\"", s3.to_string(false));
}

TEST(Structure, has_key)
{
  Structure s1;
  EXPECT_EQ(false, s1.has_key("none"));
}
