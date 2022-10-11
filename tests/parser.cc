#include <gtest/gtest.h>

#include "../src/lib/analyzer.hpp"

using namespace Parser;

TEST(Parser, CompareCheck) {
  std::vector<Token> expected{
    Keyword{.variant = KeywordE::SELECT},
    Symbol{.variant = SymbolE::ALL},
  };

  std::vector<Token> expected2{
      Keyword{.variant = KeywordE::SELECT},
      Symbol{.variant = SymbolE::ALL},
  };


  EXPECT_EQ(expected, expected2);
}

TEST(Parser, TranslateToEnums) {
  std::vector<Token> expected{
      Keyword{.variant = KeywordE::SELECT},
      Symbol{.variant = SymbolE::ALL},
  };

  std::vector<Token> result (parse("SELECT *"));

  EXPECT_EQ(expected, result);
}

TEST(Parser, TranslateToEnums2) {
  std::vector<Token> expected{
      Keyword{.variant = KeywordE::SELECT},
      Symbol{.variant = SymbolE::ALL},
      Keyword{.variant = KeywordE::FROM},
      Identifier{.value = "tabla"},
      Keyword{.variant = KeywordE::WHERE},
      Identifier{.value = "name"},
      Operator{.variant = OperatorE::EQUAL},
      String{.value = "Daniel"},
  };

  std::vector<Token> result (parse(R"(SELECT * FROM tabla WHERE name == "Daniel")"));

  EXPECT_EQ(expected, result);
}