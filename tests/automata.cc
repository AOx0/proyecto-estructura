#include <gtest/gtest.h>

#include "../src/lib/analyzer.hpp"

TEST(Automata, CreateDatabaseInvalid) {
  auto result(Automata::get_action_struct(Parser::parse("CREATE SELECT database"), "CREATE SELECT database"));

  EXPECT_TRUE(result.has_error());
}

TEST(Automata, CreateDatabaseValid) {
  auto result(Automata::get_action_struct(Parser::parse("CREATE DATABASE database"), "CREATE DATABASE database"));

  EXPECT_FALSE(result.has_error());
}

TEST(Automata, DeleteDatabaseInvalid) {
  auto result(Automata::get_action_struct(Parser::parse("DELETE SELECT database"), "DELETE SELECT database"));

  EXPECT_TRUE(result.has_error());
}

TEST(Automata, DeleteDatabaseValid) {
  auto result(Automata::get_action_struct(Parser::parse("DELETE DATABASE database"), "DELETE DATABASE database"));

  EXPECT_FALSE(result.has_error());
}
