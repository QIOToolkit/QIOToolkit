
#include "utils/log.h"

#include "gtest/gtest.h"

class LogTest : public testing::Test
{
 public:
  LogTest() = default;
  LogTest(const LogTest&) = delete;
  LogTest(LogTest&&) = delete;
  LogTest& operator=(const LogTest&) = delete;
  LogTest& operator=(LogTest&&) = delete;

  void SetUp()
  {
    utils::Logger::set_stream(&stream_);
    utils::Logger::set_level(WARN);
    utils::Logger::set_color(false);
  }

  std::string get_log() const { return stream_.str(); }

 private:
  std::stringstream stream_;
};

TEST_F(LogTest, ShowsWarning)
{
  LOG(WARN, "hello there");
#ifdef _WIN32
  EXPECT_EQ(get_log(), "[WARN] utils\\test\\log_test.cc:30: hello there\n");
#else
  EXPECT_EQ(get_log(), "[WARN] utils/test/log_test.cc:30: hello there\n");
#endif
}

class Foo
{
 public:
  void bar() { LOG(WARN, "baz"); }
  std::string get_class_name() { return __CLASS__; }
  std::string get_method_name() { return __METHOD__; }
};

TEST_F(LogTest, FromMethod)
{
  Foo foo;
  foo.bar();
#ifdef _WIN32
  EXPECT_EQ(get_log(), "[WARN] utils\\test\\log_test.cc:41: baz\n");
#else
  EXPECT_EQ(get_log(), "[WARN] utils/test/log_test.cc:41: baz\n");
#endif
  EXPECT_EQ(foo.get_class_name(), "Foo");
  EXPECT_EQ(foo.get_method_name(), "Foo::get_method_name");
}

void foo() { LOG(WARN, "bar"); }

TEST_F(LogTest, FromFunction)
{
  foo();
#ifdef _WIN32
  EXPECT_EQ(get_log(), "[WARN] utils\\test\\log_test.cc:59: bar\n");
#else
  EXPECT_EQ(get_log(), "[WARN] utils/test/log_test.cc:59: bar\n");
#endif
}

TEST_F(LogTest, ClassName) {}
