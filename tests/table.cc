#include <gtest/gtest.h>

#include "../src/lib/fm.hpp"
#include "../src/lib/table.hpp"

/*
TEST(SerDe, StructToBytes) {
  const int SIZE = 8;
  const ColumnType TYPE = ColumnType::i8;
  const bool OPTIONAL = true;

  KeyValueList<std::string, Layout> map {{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}};
  DatabaseTable input(map);

  std::vector<uint8_t> expected{'h',  'e',  'l',  'l',      'o',
                                '\0', TYPE, SIZE, OPTIONAL, '\0'};
  std::vector<uint8_t> result(input.into_vec());

  EXPECT_EQ(expected, result) << "Bad DatabaseTable struct serialization";
}

TEST(SerDe, MultipleEntry_StructToBytes) {
  const uint8_t SIZE = 8;
  const ColumnType TYPE = ColumnType::i8;
  const bool OPTIONAL = true;

  KeyValueList<std::string, Layout> map {{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}},
                                     {"hello2", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}};
  DatabaseTable input(map);

  std::vector<uint8_t> expected{
      'h', 'e', 'l', 'l', 'o', '\0', TYPE, SIZE, OPTIONAL, 'h',
      'e', 'l', 'l', 'o', '2', '\0', TYPE, SIZE, OPTIONAL, '\0'};
  std::vector<uint8_t> result(input.into_vec());

  EXPECT_EQ(expected, result) << "Bad DatabaseTable struct serialization";
}

TEST(SerDe, BytesToStruct) {
  const uint8_t SIZE = 16;
  const ColumnType TYPE = ColumnType::u16;
  const bool OPTIONAL = false;

  std::vector<uint8_t> input{'h',  'e',      'l',  'l',  'o',      '\0', TYPE,
                             SIZE, OPTIONAL, 'h',  'e',  'l',      'l',  'o',
                             '2',  '\0',     TYPE, SIZE, OPTIONAL, '\0'};

  KeyValueList<std::string, Layout> map{{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}},
                                    {"hello2", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}};
  const DatabaseTable expected(map);

  const DatabaseTable result(DatabaseTable::from_vec(input));

  EXPECT_EQ(expected, result) << "Bad DatabaseTable struct de-serialization";
}

TEST(SerDe, StructToBytesAndViceversa) {
  const int SIZE = 8;
  const ColumnType TYPE = ColumnType::i8;
  const bool OPTIONAL = true;

  KeyValueList<std::string, Layout> map{{"@$pq=", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}};
  DatabaseTable input(map);

  std::vector<uint8_t> expected{'@',  '$',  'p',  'q',      '=',
                                '\0', TYPE, SIZE, OPTIONAL, '\0'};
  std::vector<uint8_t> result(input.into_vec());

  EXPECT_EQ(expected, result) << "Bad DatabaseTable struct serialization";

  const DatabaseTable result_2(DatabaseTable::from_vec(result));

  EXPECT_EQ(input, result_2) << "Bad DatabaseTable struct de-serialization";
}
*/

TEST(SerDe, SaveLoadFromFile) {
  const int SIZE = 8;
  const ColumnType TYPE = ColumnType::i8;
  const bool OPTIONAL = true;

  KeyValueList<std::string, Layout> map{{"hello", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}},
                                    {"hello2", {.size = SIZE, .optional = OPTIONAL, .type = TYPE}}};

  DatabaseTable expected("table1", map);

  expected.to_file("./table1");

  DatabaseTable result = DatabaseTable::from_file("./table1", "table1");

  EXPECT_EQ(expected, result) << "Bad save & load";
}
