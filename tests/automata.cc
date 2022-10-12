#include <gtest/gtest.h>

#include "../src/lib/analyzer.hpp"

TEST(Automata, CreateDatabaseInvalid) {
  auto result(Automata::get_action_struct(Parser::parse("CREATE SELECT database"), "CREATE SELECT database"));

  EXPECT_TRUE(result.has_error());
}
