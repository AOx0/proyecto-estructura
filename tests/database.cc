#include <gtest/gtest.h>

#include "../src/lib/database.hpp"

TEST(DB_SerDe, StructToVec) {
  std::vector<std::uint8_t> expected{'t', 'a',  'b', 'l',  'a', '_',
                                     '1', '\0', 't', 'a',  'b', 'l',
                                     'a', '_',  '2', '\0', '\0'};

  DataBase input{.tables = {"tabla_1", "tabla_2"}};
  std::vector<std::uint8_t> result(input.into_vec());

  EXPECT_EQ(expected, result);
}

TEST(DB_SerDe, VecToStruct) {
  DataBase expected{.tables = {"tabla_1", "tabla_2"}};

  std::vector<std::uint8_t> input{'t', 'a',  'b', 'l',  'a', '_',
                                     '1', '\0', 't', 'a',  'b', 'l',
                                     'a', '_',  '2', '\0', '\0'};
  DataBase result(DataBase::from_vec(input));

  EXPECT_EQ(expected, result);
}
