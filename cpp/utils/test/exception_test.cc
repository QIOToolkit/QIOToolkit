// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/exception.h"

#include "utils/error_handling.h"
#include "gtest/gtest.h"

TEST(Exception, ReplaceEndLine)
{
  std::string message_new =
      utils::user_error("a\nb\nc", utils::Error::MemoryLimited);
  EXPECT_EQ(" _AZQ001 a b c", message_new);
}