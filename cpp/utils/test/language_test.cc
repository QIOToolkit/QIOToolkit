
#include "utils/language.h"

#include "gtest/gtest.h"

TEST(Language, IndefiniteArticle)
{
  EXPECT_EQ(utils::indef_article("elephant"), "an");
  EXPECT_EQ(utils::indef_article("Elephant"), "an");
  EXPECT_EQ(utils::indef_article("horse"), "a");
  EXPECT_EQ(utils::indef_article(""), "");
}
