#include <gtest/gtest.h>

#include "../src/lib/analyzer.hpp"

using namespace Parser;

TEST(Parser, CompareCheck) {
  std::vector<Token> expected{
    Keyword{KeywordE::SELECT},
    Symbol{SymbolE::ALL},
  };

  std::vector<Token> expected2{
      Keyword{KeywordE::SELECT},
      Symbol{SymbolE::ALL},
  };


  EXPECT_EQ(expected, expected2);
}

TEST(Parser, TranslateToEnums) {
  std::vector<Token> expected{
      Keyword{KeywordE::SELECT},
      Symbol{SymbolE::ALL},
  };

  std::vector<Token> result (parse("SELECT *"));

  EXPECT_EQ(expected, result);
}

TEST(Parser, TranslateToEnums2) {
  std::vector<Token> expected{
      Keyword{KeywordE::SELECT},
      Symbol{SymbolE::ALL},
      Keyword{KeywordE::FROM},
      Identifier{"tabla"},
      Keyword{KeywordE::WHERE},
      Identifier{"name"},
      Operator{OperatorE::EQUAL},
      String{"Daniel"},
  };

  std::vector<Token> result (parse(R"(SELECT * FROM tabla WHERE name == "Daniel")"));

  EXPECT_EQ(expected, result);
}