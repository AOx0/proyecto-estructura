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
      Identifier{Name{"tabla"}},
      Keyword{KeywordE::WHERE},
      Identifier{NameAndSub{"tabla","name"}},
      Operator{OperatorE::EQUAL},
      String{"Daniel"},
  };

  std::vector<Token> result (parse(R"(SELECT * FROM tabla WHERE tabla.name == "Daniel")"));

  EXPECT_EQ(expected, result);
}

TEST(Parser, TranslateToEnums22) {
  std::vector<Token> expected{
      Keyword{KeywordE::SELECT},
      Symbol{SymbolE::ALL},
      Keyword{KeywordE::FROM},
      Identifier{Name{"tabla"}},
      Keyword{KeywordE::WHERE},
      Identifier{Name{"tabla.name"}},
      Operator{OperatorE::EQUAL},
      String{"Daniel"},
  };

  std::vector<Token> result (parse(R"(SELECT * FROM tabla WHERE tabla.name == "Daniel")"));

  EXPECT_NE(expected, result);
}

TEST(Parser, TranslateToEnums3) {
  std::vector<Token> expected{
      Keyword{KeywordE::SELECT},
      Symbol{SymbolE::ALL},
      Keyword{KeywordE::FROM},
      Identifier{Name{"tabla"}},
      Keyword{KeywordE::WHERE},
      Unknown{"tabla.n.ame"},
      Operator{OperatorE::EQUAL},
      String{"Daniel"},
  };

  std::vector<Token> result (parse(R"(SELECT * FROM tabla WHERE tabla.n.ame == "Daniel")"));

  EXPECT_EQ(expected, result);
}