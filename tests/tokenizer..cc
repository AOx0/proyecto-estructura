#include <gtest/gtest.h>

#include "../src/lib/analyzer.hpp"

TEST(Tokenizer, Tokenize) {
  const std::string in = R"(SELECT * FROM "tabla_1" WHERE "columna_1" == 1;)";

  std::vector<std::string> expected{"SELECT", "*", "FROM", "\"tabla_1\"", "WHERE", "\"columna_1\"", "==", "1", ";"};
  std::vector<std::string> result(Tokenizer::tokenize(in));

  EXPECT_EQ(expected, result) << "Bad tokenization";
}

TEST(Tokenizer, ValidateQuery) {
  const std::string in = R"(SELECT * FROM "tabla_1" WHERE "columna_1" == 1;)";

  auto result(Tokenizer::validate(in));

  // Even though the query is not valid, it is syntactically correct
  EXPECT_TRUE(std::get<0>(result)) << "Bad tokenization";
}

TEST(Tokenizer, ValidateQuery2) {
  const std::string in = R"(SELECT * FROM tabla_1 WHERE columna_1 == "daniel";)";

  auto result(Tokenizer::validate(in));

  EXPECT_TRUE(std::get<0>(result)) << "Bad tokenization";
}
